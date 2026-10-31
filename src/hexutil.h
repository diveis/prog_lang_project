#ifndef HASHWATCH_HEXUTIL_H
#define HASHWATCH_HEXUTIL_H

#include <stddef.h>
#include <stdint.h>

/* Записывает 2*n символов + '\0' в out (размер out >= 2*n+1). */
void hex_encode(const uint8_t *in, size_t n, char *out);

/* Разбирает ровно 2*n hex-символов в n байт. 0 - успех, -1 - ошибка. */
int hex_decode(const char *hex, uint8_t *out, size_t n);

/* Сравнение за постоянное время (для MAC). 1 - равны, 0 - различны. */
int ct_equal(const void *a, const void *b, size_t n);

#endif
