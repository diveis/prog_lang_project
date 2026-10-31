#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scan.h"
#include "testutil.h"

static void write_file(const char *dir, const char *rel, const char *content)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, rel);
    FILE *f = fopen(path, "wb");
    if (f) { fputs(content, f); fclose(f); }
}

int main(void)
{
    char dir[] = "/tmp/hashwatch_scan_XXXXXX";
    CHECK(mkdtemp(dir) != NULL);

    char sub[512];
    snprintf(sub, sizeof sub, "%s/sub", dir);
    mkdir(sub, 0700);
    write_file(dir, "b.txt", "bbb");
    write_file(dir, "a.txt", "abc");
    write_file(dir, "sub/c.txt", "ccc");
    write_file(dir, "db.hwdb", "skip me");

    char link[512], target[512];
    snprintf(link, sizeof link, "%s/link", dir);
    snprintf(target, sizeof target, "%s/a.txt", dir);
    if (symlink(target, link) != 0) perror("symlink"); /* симлинки игнорируются */

    struct stat st;
    scan_skip_t skip = {0, 0, 0};
    snprintf(target, sizeof target, "%s/db.hwdb", dir);
    if (stat(target, &st) == 0) {
        skip.valid = 1; skip.dev = st.st_dev; skip.ino = st.st_ino;
    }

    entrylist_t l;
    entrylist_init(&l);
    size_t nerr = 0;
    CHECK(scan_dir(dir, &skip, &l, &nerr) == 0);
    CHECK(nerr == 0);
    CHECK(l.count == 3);
    if (l.count == 3) {
        CHECK(strcmp(l.items[0].path, "a.txt") == 0);
        CHECK(strcmp(l.items[1].path, "b.txt") == 0);
        CHECK(strcmp(l.items[2].path, "sub/c.txt") == 0);
        CHECK(l.items[0].size == 3);
        /* SHA-256("abc") начинается с 0xba7816bf */
        CHECK(l.items[0].hash[0] == 0xba && l.items[0].hash[1] == 0x78);
    }
    entrylist_free(&l);

    /* Несуществующий каталог */
    entrylist_init(&l);
    nerr = 0;
    CHECK(scan_dir("/nonexistent/hashwatch/dir", NULL, &l, &nerr) == -1);
    entrylist_free(&l);

    char cmd[600];
    snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
    if (system(cmd) != 0) perror("cleanup");
    return TEST_RESULT();
}
