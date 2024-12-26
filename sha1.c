// NOT MY CODE - SNIPED FROM ONLINE. pretty cool though
#include "sha1.h"
#include <string.h>

#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

static void sha1_transform(SHA1_CTX *ctx, const unsigned char data[]) {
    uint32_t a, b, c, d, e, t, m[80];

    for (int i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    }
    for (int i = 16; i < 80; ++i) {
        m[i] = ROTLEFT(m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16], 1);
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];

    for (int i = 0; i < 80; ++i) {
        if (i < 20) {
            t = ROTLEFT(a, 5) + ((b & c) | (~b & d)) + e + m[i] + 0x5A827999;
        } else if (i < 40) {
            t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + m[i] + 0x6ED9EBA1;
        } else if (i < 60) {
            t = ROTLEFT(a, 5) + ((b & c) | (b & d) | (c & d)) + e + m[i] + 0x8F1BBCDC;
        } else {
            t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + m[i] + 0xCA62C1D6;
        }
        e = d;
        d = c;
        c = ROTLEFT(b, 30);
        b = a;
        a = t;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

void sha1_init(SHA1_CTX *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;

    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

void sha1_update(SHA1_CTX *ctx, const unsigned char *data, size_t len) {
    size_t i, index, part_len;

    index = (ctx->count[0] >> 3) & 0x3F;

    if ((ctx->count[0] += len << 3) < (len << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += (len >> 29);

    part_len = 64 - index;

    if (len >= part_len) {
        memcpy(&ctx->buffer[index], data, part_len);
        sha1_transform(ctx, ctx->buffer);

        for (i = part_len; i + 63 < len; i += 64) {
            sha1_transform(ctx, &data[i]);
        }

        index = 0;
    } else {
        i = 0;
    }

    memcpy(&ctx->buffer[index], &data[i], len - i);
}

void sha1_final(SHA1_CTX *ctx, unsigned char hash[SHA1_BLOCK_SIZE]) {
    unsigned char bits[8];
    size_t index, pad_len;

    bits[0] = ctx->count[1] >> 24;
    bits[1] = ctx->count[1] >> 16;
    bits[2] = ctx->count[1] >> 8;
    bits[3] = ctx->count[1];
    bits[4] = ctx->count[0] >> 24;
    bits[5] = ctx->count[0] >> 16;
    bits[6] = ctx->count[0] >> 8;
    bits[7] = ctx->count[0];

    index = (ctx->count[0] >> 3) & 0x3F;
    pad_len = (index < 56) ? (56 - index) : (120 - index);

    static unsigned char PADDING[64] = { 0x80 };
    memset(&PADDING[1], 0, sizeof(PADDING) - 1);

    sha1_update(ctx, PADDING, pad_len);
    sha1_update(ctx, bits, 8);

    for (int i = 0; i < 5; ++i) {
        hash[i * 4] = (ctx->state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = ctx->state[i] & 0xFF;
    }
}
