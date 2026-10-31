#include "diff.h"

#include <stdio.h>
#include <string.h>

void diff_lists(const entrylist_t *old, const entrylist_t *cur,
                int quiet, diff_result_t *res)
{
    memset(res, 0, sizeof *res);
    size_t i = 0, j = 0;

    while (i < old->count || j < cur->count) {
        int c;
        if (i >= old->count)      c = 1;    /* остались только новые */
        else if (j >= cur->count) c = -1;   /* остались только удалённые */
        else                      c = strcmp(old->items[i].path, cur->items[j].path);

        if (c < 0) {
            res->deleted++;
            if (!quiet) printf("DELETED:  %s\n", old->items[i].path);
            i++;
        } else if (c > 0) {
            res->added++;
            if (!quiet) printf("NEW:      %s\n", cur->items[j].path);
            j++;
        } else {
            if (memcmp(old->items[i].hash, cur->items[j].hash, SHA256_DIGEST_LEN) != 0) {
                res->modified++;
                if (!quiet) printf("MODIFIED: %s\n", cur->items[j].path);
            } else {
                res->unchanged++;
            }
            i++; j++;
        }
    }
}
