#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void clean_extension(char *ext) {
    if (!ext) return;
    char *truncation_point = strpbrk(ext, "?#/");
    if (truncation_point) {
        *truncation_point = '\0';
    }
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
