#ifndef HASHWATCH_HMAC_H
#define HASHWATCH_HMAC_H

#include <stddef.h>
#include <stdint.h>

#include "sha256.h"

typedef struct {
    sha256_ctx inner;
    uint8_t    okey[SHA256_BLOCK_LEN];  
} hmac_sha256_ctx;

void hmac_sha256_init(hmac_sha256_ctx *ctx, const void *key, size_t keylen);
void hmac_sha256_update(hmac_sha256_ctx *ctx, const void *data, size_t len);
void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_LEN]);

/* Однократное вычисление. */
void hmac_sha256(const void *key, size_t keylen,
                 const void *data, size_t len,
                 uint8_t out[SHA256_DIGEST_LEN]);

#endif
