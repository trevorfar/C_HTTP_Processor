#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

typedef struct _content {
        const char *type;
        char  *dynamicUrl;
        int dynamicUrlFlag;
    } Content;

const char *valid_extensions[] = {".html", ".css", ".js", ".jpg", ".png", ".txt", ".json", ".xml"};

void parse_request(const char *request, char *method, char *url){
    sscanf(request, "%s %s", method, url);
}


void serve_file_chunked(const char *file_path, SOCKET client_socket, Content content_type) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        const char *notFound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Page Not Found</h1>";
        send(client_socket, notFound, strlen(notFound), 0);
        return;
    }

    char header[512];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type.type);
    send(client_socket, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytesRead, 0);
    }

    fclose(file);
}

char* contains_dynamic_item(const char *url) { 
    char *ptr = strstr(url, "?");
    if(ptr != NULL)
        return ptr;
    return NULL;
}

const char* parse_dynamic_url(const char *ptr) {
    static char result[256] = {0}; 
    const char *end = strchr(ptr + 1, '/'); 
    
    size_t length = end ? (size_t)(end - (ptr + 1)) : strlen(ptr + 1); 
    strncpy(result, ptr + 1, length); 
    result[length] = '\0'; 
    
    return result;
}

void clean_extension(char *ext) {
    if (!ext) return;
    char *truncation_point = strpbrk(ext, "?#/"); 
    if (truncation_point) {
        *truncation_point = '\0'; 
    }
}

void handle_request(const char *url, SOCKET client_socket) {
    char file_path[256] = "./www";

    if (strcmp(url, "/") == 0) {
        strcat(file_path, "/index.html");
    } else {
        strcat(file_path, url);
    }

    // Determine content type
    printf("FILEPATH: %s\nURL: %s\n", file_path, url);
    char *ext = strrchr(file_path, '.');
    
    if (ext) {
        clean_extension(ext);
        printf("CLEANED EXT: %s\n", ext); 
    }
    printf("EXT: %s\n", ext); 

    Content content_type;
    char dynamicUrl[256] = {0};

    content_type.type = "text/plain";
    content_type.dynamicUrl = dynamicUrl;
    content_type.dynamicUrlFlag = 0;

    if (ext) {
        if (strcmp(ext, ".html") == 0) {
            printf("Reached Here");
            char *ptr = contains_dynamic_item(url);
            if (ptr) {
                const char *parsed_dynamic_url = parse_dynamic_url(ptr);
                printf("Parsed URL: %s\n", parsed_dynamic_url);

                strncpy(dynamicUrl, parsed_dynamic_url, sizeof(dynamicUrl) - 1);
                dynamicUrl[sizeof(dynamicUrl) - 1] = '\0';

                content_type.dynamicUrlFlag = 1;
                printf("Dynamic URL: %s\n", content_type.dynamicUrl);
            }
            content_type.type = "text/html";
        } else if (strcmp(ext, ".js") == 0) {
            content_type.type = "application/javascript";
        } else if (strcmp(ext, ".css") == 0) {
            content_type.type = "text/css";
        }
    }

    serve_file_chunked(file_path, client_socket, content_type);
}



