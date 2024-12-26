#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include "server.h"
#include "dynamicparams.h"
#include "clean.h"
#include <process.h>
#include "sha1.h"
#include "base64.h"

#pragma comment(lib, "ws2_32.lib")
#define PORT 8080

const char *placeholders[2] = {"{{}}", "{{{}}}"};
void send_error_response(SOCKET client_socket, const char *message) {
    char response[512];
    snprintf(response, sizeof(response), "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n<h1>%s</h1>", message);
    send(client_socket, response, strlen(response), 0);
}


char *custom_strndup(const char *str, size_t n) {
    char *dup = malloc(n + 1);
    if (dup) {
        strncpy(dup, str, n);
        dup[n] = '\0';
    }
    return dup;
}

void serve_dynamic_html(const char *file_path, SOCKET client_socket, const DynamicParams *params) {
    char *template = load_component(file_path);
    if (!template) {
        send_error_response(client_socket, "404 Not Found");
        return;
    }

    remove_extra_whitespace(template);
    sanitize_content(template);
   
    const char *response = replace_placeholders(template, params);
    char header[512];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    send(client_socket, header, strlen(header), 0);
    send(client_socket, response, strlen(response), 0);
    free(template);
}

const char *get_content_type(const char *ext) {
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    return "text/plain";
}

void parse_request(const char *request, char *method, char *url) {
    sscanf(request, "%s %s", method, url);
}

void build_file_path(const char *url, char *file_path) {
    if(!url || !file_path){
        fprintf(stderr, "No url or file_path provided !");
        return;
    }

    const char *path_end = strchr(url, '?');
    if(path_end){
        char *temp = custom_strndup(url, path_end - url);
        snprintf(file_path, MAX_PATH_LEN, "./www%s", temp);
        temp ? free(temp) : NULL;
    }else {
        if(strcmp(url, "/") == 0){
            snprintf(file_path, MAX_PATH_LEN, "./www%s", "/index.html");
        }else {
            snprintf(file_path, MAX_PATH_LEN, "./www%s", url);
        }
    }
}

void serve_file_chunked(const char *file_path, SOCKET client_socket, const ContentType *content_type) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        const char *notFound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Page Not Found</h1>";
        printf("ERROR: File not found: %s\n", file_path);
        send(client_socket, notFound, strlen(notFound), 0);
        return;
    }

    char header[512];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type->type);
    send(client_socket, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytesRead, 0);
        printf("CHUNK SENT: %zu bytes\n", bytesRead);
    }
    fclose(file);
    printf("FILE TRANSMISSION COMPLETE: %s\n", file_path);
}


void handle_request(const char *url, SOCKET client_socket) {
    char file_path[MAX_PATH_LEN];
    DynamicParams params;
    ContentType content = { .type = "text/plain" };

    parse_dynamic_params(url, &params);    
    printf("URL2: %s\n", url);

    build_file_path(url, file_path);
    printf("FILE PATH: %s\n", file_path);

    char *ext = strrchr(file_path, '.');
    if (ext) {
        clean_extension(ext);
        printf("FILE EXTENSION: %s\n", ext);
        content.type = get_content_type(ext);
    }

    if (strcmp(content.type, "text/html") == 0) {
        serve_dynamic_html(file_path, client_socket, &params);
    } else {
        serve_file_chunked(file_path, client_socket, &content);
    }
}


void handle_client(void *socket) {
    SOCKET client_socket = (SOCKET)socket;
    char buffer[1024] = {0};

    recv(client_socket, buffer, sizeof(buffer), 0);

    if (strstr(buffer, "Upgrade: websocket")) { // apparently this is how upgrades are detected, seems odd
        printf("WebSocket request detected\n");
        handle_websocket(client_socket, buffer);
    } else {
        char method[16], url[256];
        parse_request(buffer, method, url);
        handle_request(url, client_socket);
    }

    closesocket(client_socket);
    _endthread();
}

void handle_websocket(SOCKET client_socket, const char *headers){
    // need to grab websocket key
    const char *key_header = strstr(headers, "Sec-WebSocket-Key:");
    if(!key_header){
        printf("No websocket key");
        return;
    }
    char sec_websocket_key[128];
    sscanf(key_header, "Sec-WebSocket-Key: %s", sec_websocket_key);
    const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // standard guid for websocket keys
    char concatenated[256];
    snprintf(concatenated, sizeof(concatenated), "%s%s", sec_websocket_key, guid);
    
    SHA1_CTX ctx;
    unsigned char hash[SHA1_BLOCK_SIZE];
    sha1_init(&ctx);
    sha1_update(&ctx, (unsigned char *)concatenated, strlen(concatenated));
    sha1_final(&ctx, hash);

    char accept_key[128];
    base64_encode(hash, SHA1_BLOCK_SIZE, accept_key);

    char response[256];
    printf("Sec-WebSocket-Key: %s\n", sec_websocket_key);
    printf("Concatenated: %s\n", concatenated);
    printf("Base64 Encoded Key: %s\n", accept_key);
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);
    printf("Handshake Response Sent:\n%s\n", response);

    send(client_socket, response, strlen(response), 0);

    printf("WebSocket handshake complete\n");
    websocket_communication_loop(client_socket);
}

void websocket_communication_loop(SOCKET client_socket) {
    char recv_buffer[1024];
    send_websocket_message(client_socket, "Welcome to the WebSocket server!");

    while (1) {
        int bytes_received = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
        printf("Bytes received: %d, Data: %.*s\n", bytes_received, bytes_received, recv_buffer);
        if (bytes_received == -1) {
            printf("recv() failed with error: %d\n", WSAGetLastError());
            break;
        }
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }

        unsigned char opcode = recv_buffer[0] & 0x0F;
        if (opcode == 0x8) { // Close frame
            printf("Close frame received\n");
            break;
        }

        printf("Message from client: %s\n", &recv_buffer[2]);
        send_websocket_message(client_socket, "Echo: Message received!");
    }
}

void send_websocket_message(SOCKET client_socket, const char *message) {
    size_t len = strlen(message);
    unsigned char frame[1024];
    frame[0] = 0x81; // FIN bit set, text frame
    frame[1] = (len <= 125) ? len : 126;

    size_t offset = 2;
    if (len > 125) {
        frame[1] = 126;
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        offset += 2;
    }

    memcpy(&frame[offset], message, len);
    send(client_socket, (char *)frame, len + offset, 0);
}

void websocket_printf(SOCKET client_socket, const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s", buffer); // Log to console
    send_websocket_message(client_socket, buffer); // Send to WebSocket client
}

int main() {
    clock_t begin = clock();
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // binding
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // listen
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Listen failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server is running on port %d...\n", PORT);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Took %lfs to compile+run ", time_spent);

     while ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) != INVALID_SOCKET) {
        _beginthread(handle_client, 0, (void *)new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    
    return 0;
}