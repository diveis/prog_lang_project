#include <stdlib.h>
#include <string.h>

#include "hexutil.h"
#include "sha256.h"
#include "testutil.h"

static void expect(const void *data, size_t len, const char *hex)
{
    uint8_t d[SHA256_DIGEST_LEN];
    char out[2 * SHA256_DIGEST_LEN + 1];
    sha256(data, len, d);
    hex_encode(d, sizeof d, out);
    if (strcmp(out, hex) != 0)
        fprintf(stderr, "got      %s\nexpected %s\n", out, hex);
    CHECK(strcmp(out, hex) == 0);
}

int main(void)
{
    /* Векторы FIPS 180-4 / NIST */
    expect("", 0,
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    expect("abc", 3,
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const char *m2 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    expect(m2, strlen(m2),
           "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    /* Миллион символов 'a' */
    char *big = malloc(1000000);
    memset(big, 'a', 1000000);
    expect(big, 1000000,
           "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    /* Потоковое хеширование кусками разной длины даёт тот же результат */
    sha256_ctx c;
    uint8_t d1[32], d2[32];
    sha256_init(&c);
    size_t pos = 0, step = 1;
    while (pos < 1000000) {
        size_t n = step < 1000000 - pos ? step : 1000000 - pos;
        sha256_update(&c, big + pos, n);
        pos += n;
        step = step * 3 % 211 + 1;
    }
    sha256_final(&c, d1);
    sha256(big, 1000000, d2);
    CHECK(memcmp(d1, d2, 32) == 0);
    free(big);

    /* Граничные длины вокруг размера блока / паддинга */
    for (size_t n = 54; n <= 66; n++) {
        char buf[80];
        memset(buf, 'x', sizeof buf);
        uint8_t a[32], b[32];
        sha256(buf, n, a);
        sha256_ctx t;
        sha256_init(&t);
        for (size_t i = 0; i < n; i++) sha256_update(&t, buf + i, 1);
        sha256_final(&t, b);
        CHECK(memcmp(a, b, 32) == 0);
    }

    return TEST_RESULT();
}
