#ifndef FILE_CLEANUP_H
#define FILE_CLEANUP_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PARAM_LEN 256
#define BUFFER_SIZE 8192
#define MAX_PATH_LEN 256

typedef struct {
    char key[MAX_PARAM_LEN];
    char value[MAX_PARAM_LEN];
} DynamicParam;

typedef struct {
    DynamicParam params[10]; 
    int count;
} DynamicParams;

char *load_component(const char *file_path);
const char *next_delimiter(const char *str, const char *delimiters);
void parse_dynamic_params(const char *url, DynamicParams *params);
char *escape_html(const char *input);
char *replace_placeholders(const char *template, const DynamicParams *params);

extern const char *component_placeholders[];
extern const char *component_files[];

#endif