#include "scan.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *join(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);
    char *r = malloc(la + 1 + lb + 1);
    if (!r) return NULL;
    memcpy(r, a, la);
    r[la] = '/';
    memcpy(r + la + 1, b, lb + 1);
    return r;
}

static int walk(const char *root, const char *rel, const scan_skip_t *skip,
                entrylist_t *out, size_t *nerr)
{
    char *dirpath = rel[0] ? join(root, rel) : strdup(root);
    if (!dirpath) return -1;

    DIR *d = opendir(dirpath);
    if (!d) {
        fprintf(stderr, "hashwatch: cannot open directory '%s': %s\n",
                dirpath, strerror(errno));
        (*nerr)++;
        free(dirpath);
        return 0;
    }
    free(dirpath);

    int rc = 0;
    struct dirent *de;
    while (rc == 0 && (de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        char *relchild = rel[0] ? join(rel, de->d_name) : strdup(de->d_name);
        char *full = relchild ? join(root, relchild) : NULL;
        if (!relchild || !full) {
            free(relchild);
            free(full);
            rc = -1;
            break;
        }

        struct stat st;
        if (lstat(full, &st) != 0) {
            fprintf(stderr, "hashwatch: cannot stat '%s': %s\n", full, strerror(errno));
            (*nerr)++;
        } else if (S_ISDIR(st.st_mode)) {
            rc = walk(root, relchild, skip, out, nerr);
        } else if (S_ISREG(st.st_mode)) {
            if (skip && skip->valid && st.st_dev == skip->dev && st.st_ino == skip->ino) {
                /* это файл базы - не учитываем */
            } else if (strchr(relchild, '\n')) {
                fprintf(stderr, "hashwatch: skipping '%s': newline in file name\n", full);
                (*nerr)++;
            } else {
                uint8_t h[SHA256_DIGEST_LEN];
                if (sha256_file(full, h) != 0) {
                    fprintf(stderr, "hashwatch: cannot read '%s': %s\n", full, strerror(errno));
                    (*nerr)++;
                } else if (entrylist_add(out, relchild, h, (uint64_t)st.st_size,
                                         (int64_t)st.st_mtime) != 0) {
                    rc = -1;
                }
            }
        }
        free(relchild);
        free(full);
    }
    closedir(d);
    return rc;
}

int scan_dir(const char *root, const scan_skip_t *skip,
             entrylist_t *out, size_t *nerrors)
{
    struct stat st;
    if (stat(root, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "hashwatch: '%s' is not an accessible directory\n", root);
        return -1;
    }
    if (walk(root, "", skip, out, nerrors) != 0) {
        fprintf(stderr, "hashwatch: out of memory\n");
        return -1;
    }
    entrylist_sort(out);
    return 0;
}
