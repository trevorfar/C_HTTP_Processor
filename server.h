#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "dynamicparams.h"

#define BUFFER_SIZE 8192
#define MAX_PATH_LEN 256
extern const char *placeholders[2];

typedef struct {
    const char *type;
} ContentType;

void parse_dynamic_params(const char *url, DynamicParams *params);
void serve_dynamic_html(const char *file_path, SOCKET client_socket, const DynamicParams *params);
void send_error_response(SOCKET client_socket, const char *message);
void serve_file_chunked(const char *file_path, SOCKET client_socket, const ContentType *content_type);
void handle_dynamic_url(const char *url, ContentType *content);
void clean_extension(char *ext);
char *custom_strndup(const char *str, size_t n);
void remove_extra_whitespace(char *str);
void sanitize_content(char *str);
void remove_dynamic_tags(char *html);
void parse_request(const char *request, char *method, char *url);
void build_file_path(const char *url, char *file_path);
const char *get_content_type(const char *ext);