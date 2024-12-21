#include <winsock2.h>
#include <stdio.h>
#include "server.h"

#pragma comment(lib, "ws2_32.lib")
#define PORT 8080

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
