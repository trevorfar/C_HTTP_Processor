#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void base64_encode(const unsigned char *input, int length, char *output) {
    const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int mod_table[] = {0, 2, 1}; // Maps input length % 3 to required padding
    int output_index = 0;

    for (int i = 0; i < length;) {
        uint32_t octet_a = i < length ? input[i++] : 0;
        uint32_t octet_b = i < length ? input[i++] : 0;
        uint32_t octet_c = i < length ? input[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[output_index++] = encoding_table[(triple >> 18) & 0x3F];
        output[output_index++] = encoding_table[(triple >> 12) & 0x3F];
        output[output_index++] = (i > length + mod_table[length % 3]) ? '=' : encoding_table[(triple >> 6) & 0x3F];
        output[output_index++] = (i > length + mod_table[length % 3] + 1) ? '=' : encoding_table[triple & 0x3F];
    }
    
    int padding = mod_table[length % 3];
    for (int i = 0; i < padding; i++) {
        output[output_index - 1 - i] = '=';
    }

    output[output_index] = '\0'; 
}
