#ifndef HASHWATCH_SCAN_H
#define HASHWATCH_SCAN_H

#include <stddef.h>
#include <sys/types.h>

#include "entry.h"

/* Файл, который нужно пропустить при сканировании (например, сама база). */
typedef struct {
    int   valid;
    dev_t dev;
    ino_t ino;
} scan_skip_t;

/*
 * Рекурсивно обходит каталог root и добавляет в out запись для каждого
 * обычного файла (симлинки и спецфайлы пропускаются). Пути относительные,
 * разделитель '/'. Результат отсортирован.
 *
 * *nerrors увеличивается на число файлов/каталогов, которые не удалось
 * прочитать (обход при этом продолжается).
 * Возврат: 0 - успех, -1 - root недоступен или нехватка памяти.
 */
int scan_dir(const char *root, const scan_skip_t *skip,
             entrylist_t *out, size_t *nerrors);

#endif
