#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dynamicparams.h"

const char *component_placeholders[] = {"{{HEADER}}", "{{FOOTER}}", "{{NAVBAR}}"};
const char *component_files[] = {"./www/shared/header.html", "./www/shared/footer.html", "./www/shared/navbar.html"};

char *load_component(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Component file not found: %s\n", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed for component: %s\n", file_path);
        return NULL;
    }

    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);

    printf("Loaded file: %s", content);
    return content;
}


const char *next_delimiter(const char *str, const char *delimiters){
    const char *nearest = NULL;
    for(const char *d = delimiters; *d; d++){
        const char *pos = strchr(str, *d);
        if (pos && (!nearest || pos < nearest)) {
            nearest = pos;
        }
    }
    return nearest;
}

void parse_dynamic_params(const char *url, DynamicParams *params) {
    params->count = 0;
    const char *query = strchr(url, '?');
    if (!query) {
        printf("No query string found in URL.\n");
        return;
    }
    query++;  
    const char delims[] = "&/";

    while (*query && params->count < 10) {
        printf("Current query segment: %s\n", query);

        char *sep = strchr(query, '=');
        const char *next = next_delimiter(query, delims);

        printf("query=%s, sep=%s, next=%s\n",
               query ? query : "NULL",
               sep ? sep : "NULL",
               next ? next : "NULL");

        if (!sep || (next && sep > next)) {
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

        while (*query && strchr(delims, *query)) {
            query++; 
        }
    }
    for (int j = 0; j < params->count; j++) {
        printf("Param %d: Key=%s, Value=%s\n", j, params->params[j].key, params->params[j].value);
    }
}

char *escape_html(const char *input) {
    size_t len = strlen(input);
    char *escaped = malloc(len * 5 + 1); 
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

    for (size_t i = 0; i < sizeof(component_placeholders) / sizeof(component_placeholders[0]); i++) {
        char *component_content = load_component(component_files[i]);
        if (component_content) {
            char *pos = strstr(result, component_placeholders[i]);
            if (pos) {
                char temp[BUFFER_SIZE * 2];
                snprintf(temp, sizeof(temp), "%.*s%s%s",
                         (int)(pos - result), result,
                         component_content, pos + strlen(component_placeholders[i]));
                if (strlen(temp) < sizeof(result)) {
                    strncpy(result, temp, sizeof(result) - 1);
                }
            }
            free(component_content);
        }
    }

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
