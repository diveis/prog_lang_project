#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "testutil.h"

static void fill(entrylist_t *l)
{
    uint8_t h1[32], h2[32];
    memset(h1, 0x11, 32);
    memset(h2, 0xab, 32);
    entrylist_add(l, "z/last file.txt", h1, 10, 1700000000);
    entrylist_add(l, "a.txt", h2, 0, 1600000000);
}

int main(void)
{
    char path[] = "/tmp/hashwatch_db_XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    close(fd);

    const uint8_t key[] = "secret-key";
    const uint8_t other[] = "another-key";
    entrylist_t src, dst;

    /* Без ключа: туда-обратно, пути с пробелами сохраняются */
    entrylist_init(&src); fill(&src);
    CHECK(db_save(path, &src, NULL, 0) == DB_OK);
    entrylist_init(&dst);
    CHECK(db_load(path, &dst, NULL, 0) == DB_OK);
    CHECK(dst.count == 2);
    if (dst.count == 2) {
        CHECK(strcmp(dst.items[0].path, "a.txt") == 0);
        CHECK(strcmp(dst.items[1].path, "z/last file.txt") == 0);
        CHECK(dst.items[1].size == 10 && dst.items[1].mtime == 1700000000);
        CHECK(dst.items[0].hash[0] == 0xab);
    }
    entrylist_free(&dst);
    /* Ключ передан, а база без MAC - отказ (защита от понижения защиты) */
    entrylist_init(&dst);
    CHECK(db_load(path, &dst, key, sizeof key - 1) == DB_ERR_NOMAC);
    entrylist_free(&dst);

    /* С ключом */
    CHECK(db_save(path, &src, key, sizeof key - 1) == DB_OK);
    entrylist_init(&dst);
    CHECK(db_load(path, &dst, key, sizeof key - 1) == DB_OK);
    CHECK(dst.count == 2);
    entrylist_free(&dst);

    entrylist_init(&dst);
    CHECK(db_load(path, &dst, NULL, 0) == DB_ERR_NEEDKEY);
    CHECK(db_load(path, &dst, other, sizeof other - 1) == DB_ERR_MAC);
    CHECK(dst.count == 0);
    entrylist_free(&dst);

    /* Подмена содержимого: меняем один символ хеша */
    FILE *f = fopen(path, "r+b");
    CHECK(f != NULL);
    if (f) {
        fseek(f, 14, SEEK_SET);
        int c = fgetc(f);
        fseek(f, 14, SEEK_SET);
        fputc(c == '0' ? '1' : '0', f);
        fclose(f);
    }
    entrylist_init(&dst);
    CHECK(db_load(path, &dst, key, sizeof key - 1) == DB_ERR_MAC);
    entrylist_free(&dst);

    /* Мусор и несуществующий файл */
    f = fopen(path, "wb");
    if (f) { fputs("garbage\n", f); fclose(f); }
    entrylist_init(&dst);
    CHECK(db_load(path, &dst, NULL, 0) == DB_ERR_FORMAT);
    CHECK(db_load("/nonexistent/hashwatch.db", &dst, NULL, 0) == DB_ERR_IO);
    entrylist_free(&dst);

    entrylist_free(&src);
    unlink(path);
    return TEST_RESULT();
}
