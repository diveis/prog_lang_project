#ifndef HASHWATCH_ENTRY_H
#define HASHWATCH_ENTRY_H

#include <stddef.h>
#include <stdint.h>

#include "sha256.h"

typedef struct {
    char    *path;
    uint8_t  hash[SHA256_DIGEST_LEN];
    uint64_t size;
    int64_t  mtime;
} entry_t;

typedef struct {
    entry_t *items;
    size_t   count;
    size_t   cap;
} entrylist_t;

void entrylist_init(entrylist_t *l);
void entrylist_free(entrylist_t *l);

/* запись 0 - успех, -1 - нет памяти */
int entrylist_add(entrylist_t *l, const char *path,
                  const uint8_t hash[SHA256_DIGEST_LEN],
                  uint64_t size, int64_t mtime);

void entrylist_sort(entrylist_t *l);

#endif
