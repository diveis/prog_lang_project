#ifndef HASHWATCH_SCAN_H
#define HASHWATCH_SCAN_H

#include <stddef.h>
#include <sys/types.h>

#include "entry.h"

/* файл, который нужно пропустить при сканировании (например, сама база) */
typedef struct {
    int   valid;
    dev_t dev;
    ino_t ino;
} scan_skip_t;

int scan_dir(const char *root, const scan_skip_t *skip,
             entrylist_t *out, size_t *nerrors);

#endif
