#include <winsock2.h>
#include <stdio.h>
#include "server.h"

#pragma comment(lib, "ws2_32.lib")
#define PORT 8080

void handle_request(const char *url, SOCKET client_socket) {
    printf("REQUEST URL: %s\n", url);

    char file_path[MAX_PATH_LEN];
    DynamicParams params;
    ContentType content = { .type = "text/plain" };

    parse_dynamic_params(url, &params);

    for (int i = 0; i < 1; i++) {
        printf("Key: %s, Value: %s\n", params.params[i].key, params.params[i].value);
    }

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

void send_error_response(SOCKET client_socket, const char *message) {
    char response[512];
    snprintf(response, sizeof(response), "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n<h1>%s</h1>", message);
    send(client_socket, response, strlen(response), 0);
}

int main() {
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
