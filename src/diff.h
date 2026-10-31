#ifndef HASHWATCH_DIFF_H
#define HASHWATCH_DIFF_H

#include <stddef.h>

#include "entry.h"

typedef struct {
    size_t modified;
    size_t deleted;
    size_t added;
    size_t unchanged;
} diff_result_t;

/*
 * Сравнивает сохранённую базу (old) с текущим состоянием (cur).
 * Оба списка должны быть отсортированы по пути. Для каждого отличия
 * печатает строку в stdout:  MODIFIED: / DELETED: / NEW: <путь>
 * (при quiet != 0 строки не печатаются, только счётчики).
 */
void diff_lists(const entrylist_t *old, const entrylist_t *cur,
                int quiet, diff_result_t *res);

static inline size_t diff_total_changes(const diff_result_t *r)
{
    return r->modified + r->deleted + r->added;
}

#endif
