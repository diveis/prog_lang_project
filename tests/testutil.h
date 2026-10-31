#ifndef HASHWATCH_TESTUTIL_H
#define HASHWATCH_TESTUTIL_H

#include <stdio.h>

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_failures++;                                                  \
        }                                                                  \
    } while (0)

#define TEST_RESULT()                                                      \
    (g_failures ? (fprintf(stderr, "%d failure(s)\n", g_failures), 1)      \
                : (puts("OK"), 0))

#endif
