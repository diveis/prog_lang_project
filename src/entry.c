#include "entry.h"

#include <stdlib.h>
#include <string.h>

void entrylist_init(entrylist_t *l)
{
    l->items = NULL;
    l->count = 0;
    l->cap = 0;
}

void entrylist_free(entrylist_t *l)
{
    for (size_t i = 0; i < l->count; i++)
        free(l->items[i].path);
    free(l->items);
    entrylist_init(l);
}

int entrylist_add(entrylist_t *l, const char *path,
                  const uint8_t hash[SHA256_DIGEST_LEN],
                  uint64_t size, int64_t mtime)
{
    if (l->count == l->cap) {
        size_t ncap = l->cap ? l->cap * 2 : 64;
        entry_t *n = realloc(l->items, ncap * sizeof *n);
        if (!n) return -1;
        l->items = n;
        l->cap = ncap;
    }
    char *p = strdup(path);
    if (!p) return -1;

    entry_t *e = &l->items[l->count++];
    e->path = p;
    memcpy(e->hash, hash, SHA256_DIGEST_LEN);
    e->size = size;
    e->mtime = mtime;
    return 0;
}

static int cmp_entry(const void *a, const void *b)
{
    return strcmp(((const entry_t *)a)->path, ((const entry_t *)b)->path);
}

void entrylist_sort(entrylist_t *l)
{
    if (l->count > 1)
        qsort(l->items, l->count, sizeof(entry_t), cmp_entry);
}
