#include "sha1.h"
#include <stdio.h>
#include <string.h>

int main() {
    const char *message = "Hello, SHA-1!";
    unsigned char hash[SHA1_BLOCK_SIZE];

    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (const unsigned char *)message, strlen(message));
    sha1_final(&ctx, hash);

    printf("SHA1 hash: ");
    for (int i = 0; i < SHA1_BLOCK_SIZE; ++i) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    return 0;
}
