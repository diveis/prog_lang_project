/*
 * hashwatch - проверка целостности файлов по базе хешей SHA-256 (FIPS 180-4)
 * с опциональной защитой самой базы через HMAC (RFC 2104).
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "db.h"
#include "diff.h"
#include "entry.h"
#include "scan.h"

#define HASHWATCH_VERSION "0.1.0"
#define MAX_KEY_SIZE (1 << 20)

/* Коды возврата */
enum { EXIT_CLEAN = 0, EXIT_CHANGED = 1, EXIT_ERROR = 2 };

typedef struct {
    const char *cmd;
    const char *dir;
    const char *db;
    const char *keyfile;
    int force;
    int quiet;
} options_t;

static void usage(FILE *out)
{
    fprintf(out,
        "hashwatch " HASHWATCH_VERSION " - file integrity checker (SHA-256 / HMAC-SHA256)\n"
        "\n"
        "Usage:\n"
        "  hashwatch init   <dir> -d <db> [-k <keyfile>] [-f]   create hash database\n"
        "  hashwatch check  <dir> -d <db> [-k <keyfile>] [-q]   report changes\n"
        "  hashwatch update <dir> -d <db> [-k <keyfile>]        report changes and refresh database\n"
        "\n"
        "Options:\n"
        "  -d <db>       path to the database file (required)\n"
        "  -k <keyfile>  HMAC key file: protects the database against tampering\n"
        "  -f            init: overwrite an existing database\n"
        "  -q            check: print only the summary\n"
        "  -h, --help    show this help\n"
        "  --version     show version\n"
        "\n"
        "Exit codes: 0 - no changes, 1 - changes found, 2 - error\n");
}

static int parse_args(int argc, char **argv, options_t *o)
{
    memset(o, 0, sizeof *o);
    if (argc < 2) return -1;
    o->cmd = argv[1];

    for (int i = 2; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-d") || !strcmp(a, "-k")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "hashwatch: option %s requires an argument\n", a);
                return -1;
            }
            if (a[1] == 'd') o->db = argv[++i]; else o->keyfile = argv[++i];
        } else if (!strcmp(a, "-f")) {
            o->force = 1;
        } else if (!strcmp(a, "-q")) {
            o->quiet = 1;
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "hashwatch: unknown option '%s'\n", a);
            return -1;
        } else if (!o->dir) {
            o->dir = a;
        } else {
            fprintf(stderr, "hashwatch: unexpected argument '%s'\n", a);
            return -1;
        }
    }
    if (!o->dir || !o->db) {
        fprintf(stderr, "hashwatch: directory and -d <db> are required\n");
        return -1;
    }
    return 0;
}

static uint8_t *read_key(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "hashwatch: cannot open key file '%s': %s\n", path, strerror(errno));
        return NULL;
    }
    uint8_t *buf = malloc(MAX_KEY_SIZE + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, MAX_KEY_SIZE + 1, f);
    fclose(f);
    if (n == 0 || n > MAX_KEY_SIZE) {
        fprintf(stderr, "hashwatch: key file must be 1..%d bytes\n", MAX_KEY_SIZE);
        free(buf);
        return NULL;
    }
    *len = n;
    return buf;
}

static void print_summary(const diff_result_t *r)
{
    printf("Summary: %zu modified, %zu deleted, %zu new, %zu unchanged\n",
           r->modified, r->deleted, r->added, r->unchanged);
}

static int do_scan(const options_t *o, entrylist_t *cur, size_t *nerr)
{
    scan_skip_t skip = {0, 0, 0};
    struct stat st;
    if (stat(o->db, &st) == 0) {      /* не учитываем саму базу, если она в каталоге */
        skip.valid = 1; skip.dev = st.st_dev; skip.ino = st.st_ino;
    }
    return scan_dir(o->dir, &skip, cur, nerr);
}

static int cmd_init(const options_t *o, const uint8_t *key, size_t keylen)
{
    struct stat st;
    if (!o->force && stat(o->db, &st) == 0) {
        fprintf(stderr, "hashwatch: '%s' already exists (use -f to overwrite, or 'update')\n", o->db);
        return EXIT_ERROR;
    }
    entrylist_t cur;
    entrylist_init(&cur);
    size_t nerr = 0;
    int rc = EXIT_ERROR;
    if (do_scan(o, &cur, &nerr) == 0) {
        db_status s = db_save(o->db, &cur, key, keylen);
        if (s == DB_OK) {
            printf("Indexed %zu files -> %s%s\n", cur.count, o->db,
                   key ? " (HMAC-protected)" : "");
            if (nerr) fprintf(stderr, "hashwatch: warning: %zu item(s) could not be read\n", nerr);
            rc = EXIT_CLEAN;
        } else {
            fprintf(stderr, "hashwatch: cannot write '%s': %s\n", o->db, db_strerror(s));
        }
    }
    entrylist_free(&cur);
    return rc;
}

static int cmd_check(const options_t *o, const uint8_t *key, size_t keylen, int refresh)
{
    entrylist_t old, cur;
    entrylist_init(&old);
    entrylist_init(&cur);
    int rc = EXIT_ERROR;

    db_status s = db_load(o->db, &old, key, keylen);
    if (s != DB_OK) {
        fprintf(stderr, "hashwatch: cannot load '%s': %s\n", o->db, db_strerror(s));
        goto out;
    }
    size_t nerr = 0;
    if (do_scan(o, &cur, &nerr) != 0) goto out;

    diff_result_t r;
    diff_lists(&old, &cur, o->quiet, &r);
    print_summary(&r);
    rc = diff_total_changes(&r) ? EXIT_CHANGED : EXIT_CLEAN;

    if (nerr) {
        fprintf(stderr, "hashwatch: warning: %zu item(s) could not be read\n", nerr);
        if (rc == EXIT_CLEAN) rc = EXIT_ERROR;   /* нечитаемый файл = проверка неполная */
    }
    if (refresh) {
        s = db_save(o->db, &cur, key, keylen);
        if (s != DB_OK) {
            fprintf(stderr, "hashwatch: cannot write '%s': %s\n", o->db, db_strerror(s));
            rc = EXIT_ERROR;
        } else {
            printf("Database updated.\n");
        }
    }
out:
    entrylist_free(&old);
    entrylist_free(&cur);
    return rc;
}

int main(int argc, char **argv)
{
    if (argc >= 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return EXIT_CLEAN;
    }
    if (argc >= 2 && !strcmp(argv[1], "--version")) {
        puts("hashwatch " HASHWATCH_VERSION);
        return EXIT_CLEAN;
    }

    options_t o;
    if (parse_args(argc, argv, &o) != 0) {
        usage(stderr);
        return EXIT_ERROR;
    }
    if (strcmp(o.cmd, "init") && strcmp(o.cmd, "check") && strcmp(o.cmd, "update")) {
        fprintf(stderr, "hashwatch: unknown command '%s'\n", o.cmd);
        usage(stderr);
        return EXIT_ERROR;
    }

    uint8_t *key = NULL;
    size_t keylen = 0;
    if (o.keyfile && !(key = read_key(o.keyfile, &keylen)))
        return EXIT_ERROR;

    int rc;
    if (!strcmp(o.cmd, "init"))
        rc = cmd_init(&o, key, keylen);
    else
        rc = cmd_check(&o, key, keylen, !strcmp(o.cmd, "update"));

    if (key) { memset(key, 0, keylen); free(key); }
    return rc;
}
