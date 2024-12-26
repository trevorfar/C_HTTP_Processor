// sha1.h
#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>
#include <stddef.h>

#define SHA1_BLOCK_SIZE 20 // SHA1 outputs 20 bytes

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, const unsigned char *data, size_t len);
void sha1_final(SHA1_CTX *ctx, unsigned char hash[SHA1_BLOCK_SIZE]);

#endif
