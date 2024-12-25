#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "dynamicparams.h"

#define BUFFER_SIZE 8192
#define MAX_PATH_LEN 256
const char *placeholders[2] = {"{{}}", "{{{}}}"};

typedef struct {
    const char *type;
} ContentType;

void parse_dynamic_params(const char *url, DynamicParams *params);
char *replace_placeholders(const char *template, const DynamicParams *params);
void serve_dynamic_html(const char *file_path, SOCKET client_socket, const DynamicParams *params);
void send_error_response(SOCKET client_socket, const char *message);
void serve_file_chunked(const char *file_path, SOCKET client_socket, const ContentType *content_type);
void handle_dynamic_url(const char *url, ContentType *content);

void clean_extension(char *ext) {
    if (!ext) return;
    char *truncation_point = strpbrk(ext, "?#/");
    if (truncation_point) {
        *truncation_point = '\0';
    }
}

char *custom_strndup(const char *str, size_t n) {
    char *dup = malloc(n + 1);
    if (dup) {
        strncpy(dup, str, n);
        dup[n] = '\0';
    }
    return dup;
}


void remove_extra_whitespace(char *str) {
    char *write_ptr = str;
    char prev_char = '\0';

    for (char *read_ptr = str; *read_ptr; read_ptr++) {
        if (isspace(*read_ptr)) {
            if (prev_char && !isspace(prev_char)) {
                *write_ptr++ = ' ';
            }
        } else {
            *write_ptr++ = *read_ptr;
        }
        prev_char = *read_ptr;
    }
    *write_ptr = '\0'; 
}

void sanitize_content(char *str) {
    char *write_ptr = str;
    for (char *read_ptr = str; *read_ptr; ++read_ptr) {
        if (*read_ptr >= 32 && *read_ptr <= 126) {  
            *write_ptr++ = *read_ptr;
        }
    }
    *write_ptr = '\0';
}

void remove_dynamic_tags(char *html) {
    char *result = malloc(strlen(html) + 1);
    if (!result) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }
    char *src = html;
    char *dst = result;
    while (*src) {
        if (*src == '{' && *(src + 1) == '{') {
            src += 2;  
            while (*src && !(*src == '}' && *(src + 1) == '}')) {
                src++;
            }
            if (*src) src += 2;  
        } else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
    strcpy(html, result);
    free(result);
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

    // if(params->count == 0){
    //         remove_dynamic_tags(response);
    // }

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


