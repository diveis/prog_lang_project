#include "hexutil.h"

void hex_encode(const uint8_t *in, size_t n, char *out)
{
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i]     = digits[in[i] >> 4];
        out[2 * i + 1] = digits[in[i] & 0x0f];
    }
    out[2 * n] = '\0';
}

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int hex_decode(const char *hex, uint8_t *out, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int hi = hex_val(hex[2 * i]);
        if (hi < 0) return -1;
        int lo = hex_val(hex[2 * i + 1]);
        if (lo < 0) return -1;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

int ct_equal(const void *a, const void *b, size_t n)
{
    const volatile uint8_t *pa = a, *pb = b;
    uint8_t diff = 0;
    for (size_t i = 0; i < n; i++)
        diff |= (uint8_t)(pa[i] ^ pb[i]);
    return diff == 0;
}
