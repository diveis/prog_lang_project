#include <string.h>

#include "diff.h"
#include "testutil.h"

static void add(entrylist_t *l, const char *p, uint8_t v)
{
    uint8_t h[32];
    memset(h, v, 32);
    entrylist_add(l, p, h, 1, 0);
}

int main(void)
{
    entrylist_t o, c;
    entrylist_init(&o); entrylist_init(&c);

    add(&o, "a", 1);  add(&c, "a", 1);   /* без изменений */
    add(&o, "b", 2);  add(&c, "b", 9);   /* изменён */
    add(&o, "c", 3);                     /* удалён */
    add(&c, "d", 4);                     /* новый */
    add(&c, "e", 5);                     /* новый */
    add(&o, "z", 6);                     /* удалён в конце */

    diff_result_t r;
    diff_lists(&o, &c, 1, &r);
    CHECK(r.unchanged == 1);
    CHECK(r.modified == 1);
    CHECK(r.deleted == 2);
    CHECK(r.added == 2);
    CHECK(diff_total_changes(&r) == 5);

    entrylist_free(&o); entrylist_free(&c);

    /* Пустые списки */
    entrylist_init(&o); entrylist_init(&c);
    diff_lists(&o, &c, 1, &r);
    CHECK(diff_total_changes(&r) == 0 && r.unchanged == 0);
    entrylist_free(&o); entrylist_free(&c);

    return TEST_RESULT();
}
