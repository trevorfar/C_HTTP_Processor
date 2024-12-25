#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include "server.h"
#include "dynamicparams.h"
#include "clean.h"

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
    FILE *file = fopen(file_path, "r");
    if (!file) {
        send_error_response(client_socket, "404 Not Found");
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *template = malloc(file_size + 1);
    if (!template) {
        fclose(file);
        send_error_response(client_socket, "500 Internal Server Error");
        return;
    }
    fread(template, 1, file_size, file);
    template[file_size] = '\0';
    fclose(file);

    remove_extra_whitespace(template);
    sanitize_content(template);
   
    const char *response = replace_placeholders(template, params);
    free(template);
    char header[512];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    send(client_socket, header, strlen(header), 0);
    send(client_socket, response, strlen(response), 0);
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
    printf("HEADER SENT:\n%s", header);

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
    printf("REQUEST URL: %s\n", url);

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
        char buffer[1024] = {0};
        recv(new_socket, buffer, sizeof(buffer), 0);
        char method[16], url[256], response[2048] = {0};
        parse_request(buffer, method, url);
        handle_request(url, new_socket);


        send(new_socket, response, strlen(response), 0);
        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    
    return 0;
}