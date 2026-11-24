#ifndef HASHWATCH_DB_H
#define HASHWATCH_DB_H

#include <stddef.h>
#include <stdint.h>

#include "entry.h"

typedef enum {
    DB_OK = 0,
    DB_ERR_IO,        /* ошибка чтения/записи файла */
    DB_ERR_FORMAT,    /* повреждённый или неизвестный формат */
    DB_ERR_NEEDKEY,   /* база защищена HMAC, а ключ не передан */
    DB_ERR_NOMAC,     /* ключ передан, а база не защищена (возможна подмена) */
    DB_ERR_MAC,       /* HMAC не совпал: база изменена или неверный ключ */
    DB_ERR_NOMEM
} db_status;

const char *db_strerror(db_status s);

/*
 * сохраняет список в файл (временный файл + rename).
 * если key != NULL, в конец добавляется HMAC-SHA256 всего содержимого.
 */
db_status db_save(const char *path, const entrylist_t *list,
                  const uint8_t *key, size_t keylen);

/*
 * если в файле есть MAC - обязателен key и он проверяется до разбора содержимого */
db_status db_load(const char *path, entrylist_t *out,
                  const uint8_t *key, size_t keylen);

#endif
