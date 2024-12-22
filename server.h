#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define MAX_PATH_LEN 256
#define MAX_PARAM_LEN 256

typedef struct {
    char key[MAX_PARAM_LEN];
    char value[MAX_PARAM_LEN];
} DynamicParam;

typedef struct {
    DynamicParam params[10]; 
    int count;
} DynamicParams;

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


void parse_dynamic_params(const char *url, DynamicParams *params) {
    int i = 0;
    params->count = 0;

    const char *query = strchr(url, '?');
    if (!query) {
        printf("No query string found in URL.\n");
        return;
    }
    query++;  

    int max_iterations = 20; 
    while (*query && params->count < 10 && max_iterations--) {
        printf("Current query segment: %s\n", query);

        char *sep = strchr(query, '=');
        char *next = strchr(query, '&');

        printf("query=%s, sep=%s, next=%s\n",
               query ? query : "NULL",
               sep ? sep : "NULL",
               next ? next : "NULL");

        if (!sep) {
            printf("No '=' found, skipping to next segment.\n");
            break;
        }

        size_t key_len = sep - query;
        size_t val_len = next ? (size_t)(next - sep - 1) : strlen(sep + 1);

        if (key_len > MAX_PARAM_LEN - 1) key_len = MAX_PARAM_LEN - 1;
        if (val_len > MAX_PARAM_LEN - 1) val_len = MAX_PARAM_LEN - 1;

        strncpy(params->params[params->count].key, query, key_len);
        params->params[params->count].key[key_len] = '\0';

        strncpy(params->params[params->count].value, sep + 1, val_len);
        params->params[params->count].value[val_len] = '\0';

        params->count++;
        printf("Extracted param %d: key=%s, value=%s\n",
               params->count - 1,
               params->params[params->count - 1].key,
               params->params[params->count - 1].value);
        if (next) {
            query = next + 1;
        } else {
            break; 
        }

        if (query && (*query == '&' || *query == '/')) {
            query++;
        }
    }

    if (max_iterations == 0) {
        printf("Loop terminated due to safety limit.\n");
    }

    printf("Final count of parameters: %d\n", params->count);
    for (int j = 0; j < params->count; j++) {
        printf("Param %d: Key=%s, Value=%s\n", j, params->params[j].key, params->params[j].value);
    }
}



char *escape_html(const char *input) {
    // Estimate maximum size needed for the escaped string
    size_t len = strlen(input);
    char *escaped = malloc(len * 5 + 1); // Worst case: every character needs to be escaped
    if (!escaped) return NULL;

    char *out = escaped;
    for (size_t i = 0; i < len; ++i) {
        switch (input[i]) {
            case '<': strcpy(out, "&lt;"); out += 4; break;
            case '>': strcpy(out, "&gt;"); out += 4; break;
            case '&': strcpy(out, "&amp;"); out += 5; break;
            case '"': strcpy(out, "&quot;"); out += 6; break;
            case '\'': strcpy(out, "&apos;"); out += 6; break;
            default: *out++ = input[i]; break;
        }
    }
    *out = '\0'; 
    return escaped;
}

char *replace_placeholders(const char *template, const DynamicParams *params) {
    static char result[BUFFER_SIZE * 2];
    strncpy(result, template, sizeof(result) - 1);

    for (int i = 0; i < params->count; i++) {
        char placeholder[MAX_PARAM_LEN + 4];
        snprintf(placeholder, sizeof(placeholder), "{{%s}}", params->params[i].key);

        char *pos = strstr(result, placeholder);
        if (pos) {
            char *escaped_value = escape_html(params->params[i].value);
            if (escaped_value) {
                char temp[BUFFER_SIZE * 2];
                snprintf(temp, sizeof(temp), "%.*s%s%s",
                         (int)(pos - result), result,
                         escaped_value, pos + strlen(placeholder));
                if (strlen(temp) < sizeof(result)) {
                    strncpy(result, temp, sizeof(result) - 1);
                }
                free(escaped_value);  
            }
        }
    }

    return result;
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
    const char *path_end = strchr(url, '?');
    char *temp = custom_strndup(url, path_end - url);
    snprintf(file_path, MAX_PATH_LEN, "./www%s", temp);
    free(temp);
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


