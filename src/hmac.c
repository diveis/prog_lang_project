#include "hmac.h"

#include <string.h>

void hmac_sha256_init(hmac_sha256_ctx *ctx, const void *key, size_t keylen)
{
    uint8_t k[SHA256_BLOCK_LEN];
    uint8_t ipad[SHA256_BLOCK_LEN];

    memset(k, 0, sizeof k);
    if (keylen > SHA256_BLOCK_LEN)
        sha256(key, keylen, k);          /* длинный ключ сначала хешируется */
    else if (keylen > 0)
        memcpy(k, key, keylen);          /* короткий дополняется нулями */

    for (size_t i = 0; i < SHA256_BLOCK_LEN; i++) {
        ipad[i]      = k[i] ^ 0x36;
        ctx->okey[i] = k[i] ^ 0x5c;
    }

    sha256_init(&ctx->inner);
    sha256_update(&ctx->inner, ipad, sizeof ipad);

    memset(k, 0, sizeof k);
    memset(ipad, 0, sizeof ipad);
}

void hmac_sha256_update(hmac_sha256_ctx *ctx, const void *data, size_t len)
{
    sha256_update(&ctx->inner, data, len);
}

void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_LEN])
{
    uint8_t inner_digest[SHA256_DIGEST_LEN];
    sha256_ctx outer;

    sha256_final(&ctx->inner, inner_digest);

    sha256_init(&outer);
    sha256_update(&outer, ctx->okey, sizeof ctx->okey);
    sha256_update(&outer, inner_digest, sizeof inner_digest);
    sha256_final(&outer, out);

    memset(inner_digest, 0, sizeof inner_digest);
    memset(ctx, 0, sizeof *ctx);
}

void hmac_sha256(const void *key, size_t keylen,
                 const void *data, size_t len,
                 uint8_t out[SHA256_DIGEST_LEN])
{
    hmac_sha256_ctx ctx;
    hmac_sha256_init(&ctx, key, keylen);
    hmac_sha256_update(&ctx, data, len);
    hmac_sha256_final(&ctx, out);
}
