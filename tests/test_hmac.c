#include <string.h>

#include "hexutil.h"
#include "hmac.h"
#include "testutil.h"

static void expect(const void *key, size_t klen, const void *msg, size_t mlen,
                   const char *hex)
{
    uint8_t d[32];
    char out[65];
    hmac_sha256(key, klen, msg, mlen, d);
    hex_encode(d, 32, out);
    if (strcmp(out, hex) != 0)
        fprintf(stderr, "got      %s\nexpected %s\n", out, hex);
    CHECK(strcmp(out, hex) == 0);
}

int main(void)
{
    /* RFC 4231, тест-кейс 1 */
    uint8_t key1[20];
    memset(key1, 0x0b, sizeof key1);
    expect(key1, sizeof key1, "Hi There", 8,
           "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");

    /* RFC 4231, тест-кейс 2 */
    expect("Jefe", 4, "what do ya want for nothing?", 28,
           "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");

    /* RFC 4231, тест-кейс 3: ключ 20 x 0xaa, данные 50 x 0xdd */
    uint8_t key3[20], data3[50];
    memset(key3, 0xaa, sizeof key3);
    memset(data3, 0xdd, sizeof data3);
    expect(key3, sizeof key3, data3, sizeof data3,
           "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");

    /* RFC 4231, тест-кейс 6: ключ длиннее блока (131 байт) */
    uint8_t key6[131];
    memset(key6, 0xaa, sizeof key6);
    const char *m6 = "Test Using Larger Than Block-Size Key - Hash Key First";
    expect(key6, sizeof key6, m6, strlen(m6),
           "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");

    /* Потоковый режим совпадает с однократным */
    hmac_sha256_ctx c;
    uint8_t a[32], b[32];
    hmac_sha256_init(&c, "Jefe", 4);
    hmac_sha256_update(&c, "what do ya ", 11);
    hmac_sha256_update(&c, "want for nothing?", 17);
    hmac_sha256_final(&c, a);
    hmac_sha256("Jefe", 4, "what do ya want for nothing?", 28, b);
    CHECK(memcmp(a, b, 32) == 0);

    return TEST_RESULT();
}
