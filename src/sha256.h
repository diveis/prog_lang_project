#ifndef HASHWATCH_SHA256_H
#define HASHWATCH_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_LEN 32
#define SHA256_BLOCK_LEN  64

/* Реализация SHA-256 по FIPS 180-4. */
typedef struct {
    uint32_t state[8];
    uint64_t total;               /* всего байт обработано */
    uint8_t  buf[SHA256_BLOCK_LEN];
    size_t   buflen;
} sha256_ctx;

void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const void *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_LEN]);

/* Однократное хеширование буфера. */
void sha256(const void *data, size_t len, uint8_t out[SHA256_DIGEST_LEN]);

/* Хеш файла (потоково). 0 - успех, -1 - ошибка чтения (errno выставлен). */
int sha256_file(const char *path, uint8_t out[SHA256_DIGEST_LEN]);

#endif
