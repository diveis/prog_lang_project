#include "db.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "hexutil.h"
#include "hmac.h"

#define DB_MAGIC "HASHWATCH 1"

const char *db_strerror(db_status s)
{
    switch (s) {
    case DB_OK:          return "ok";
    case DB_ERR_IO:      return "I/O error";
    case DB_ERR_FORMAT:  return "database is corrupted or has unknown format";
    case DB_ERR_NEEDKEY: return "database is HMAC-protected: a key file (-k) is required";
    case DB_ERR_NOMAC:   return "database has no HMAC but a key was given (possible tampering)";
    case DB_ERR_MAC:     return "HMAC mismatch: database was modified or the key is wrong";
    case DB_ERR_NOMEM:   return "out of memory";
    }
    return "unknown error";
}

db_status db_save(const char *path, const entrylist_t *list,
                  const uint8_t *key, size_t keylen)
{
    char *body = NULL;
    size_t blen = 0;
    FILE *ms = open_memstream(&body, &blen);
    if (!ms) return DB_ERR_NOMEM;

    fprintf(ms, "%s\n", DB_MAGIC);
    for (size_t i = 0; i < list->count; i++) {
        const entry_t *e = &list->items[i];
        char hex[2 * SHA256_DIGEST_LEN + 1];
        hex_encode(e->hash, SHA256_DIGEST_LEN, hex);
        fprintf(ms, "%s %" PRIu64 " %" PRId64 " %s\n", hex, e->size, e->mtime, e->path);
    }
    if (fclose(ms) != 0) { free(body); return DB_ERR_NOMEM; }

    char footer[80];
    if (key) {
        uint8_t mac[SHA256_DIGEST_LEN];
        char machex[2 * SHA256_DIGEST_LEN + 1];
        hmac_sha256(key, keylen, body, blen, mac);
        hex_encode(mac, sizeof mac, machex);
        snprintf(footer, sizeof footer, "MAC %s\n", machex);
    } else {
        snprintf(footer, sizeof footer, "END\n");
    }

    size_t plen = strlen(path);
    char *tmp = malloc(plen + 5);
    if (!tmp) { free(body); return DB_ERR_NOMEM; }
    memcpy(tmp, path, plen);
    memcpy(tmp + plen, ".tmp", 5);

    db_status st = DB_ERR_IO;
    FILE *f = fopen(tmp, "wb");
    if (f) {
        int ok = fwrite(body, 1, blen, f) == blen &&
                 fputs(footer, f) >= 0;
        ok = (fclose(f) == 0) && ok;
        if (ok && chmod(tmp, 0600) == 0 && rename(tmp, path) == 0)
            st = DB_OK;
        else
            remove(tmp);
    }
    free(tmp);
    free(body);
    return st;
}

/* Читает весь файл в буфер (+ завершающий '\0'). */
static char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    size_t cap = 1 << 16, n = 0;
    char *buf = malloc(cap + 1);
    while (buf) {
        size_t r = fread(buf + n, 1, cap - n, f);
        n += r;
        if (n < cap) break;
        cap *= 2;
        char *nb = realloc(buf, cap + 1);
        if (!nb) { free(buf); buf = NULL; break; }
        buf = nb;
    }
    int err = ferror(f);
    fclose(f);
    if (!buf) return NULL;
    if (err) { free(buf); return NULL; }
    buf[n] = '\0';
    *len = n;
    return buf;
}

static db_status parse_line(char *line, entrylist_t *out)
{
    uint8_t hash[SHA256_DIGEST_LEN];
    if (strlen(line) < 2 * SHA256_DIGEST_LEN + 1 ||
        hex_decode(line, hash, SHA256_DIGEST_LEN) != 0 ||
        line[2 * SHA256_DIGEST_LEN] != ' ')
        return DB_ERR_FORMAT;

    char *p = line + 2 * SHA256_DIGEST_LEN + 1, *end;
    errno = 0;
    uint64_t size = strtoull(p, &end, 10);
    if (end == p || *end != ' ' || errno) return DB_ERR_FORMAT;
    p = end + 1;
    int64_t mtime = strtoll(p, &end, 10);
    if (end == p || *end != ' ' || errno) return DB_ERR_FORMAT;
    p = end + 1;
    if (*p == '\0') return DB_ERR_FORMAT;

    return entrylist_add(out, p, hash, size, mtime) == 0 ? DB_OK : DB_ERR_NOMEM;
}

db_status db_load(const char *path, entrylist_t *out,
                  const uint8_t *key, size_t keylen)
{
    size_t len;
    char *buf = slurp(path, &len);
    if (!buf) return DB_ERR_IO;

    db_status st = DB_ERR_FORMAT;
    if (len == 0 || buf[len - 1] != '\n') goto done;

    /* Последняя строка - футер (MAC или END), всё до неё - тело. */
    size_t p = len - 1;
    while (p > 0 && buf[p - 1] != '\n') p--;
    size_t body_len = p;
    if (body_len == 0) goto done;
    buf[len - 1] = '\0';
    const char *footer = buf + p;

    if (strncmp(footer, "MAC ", 4) == 0) {
        if (!key) { st = DB_ERR_NEEDKEY; goto done; }
        uint8_t stored[SHA256_DIGEST_LEN], calc[SHA256_DIGEST_LEN];
        if (strlen(footer + 4) != 2 * SHA256_DIGEST_LEN ||
            hex_decode(footer + 4, stored, sizeof stored) != 0)
            goto done;
        hmac_sha256(key, keylen, buf, body_len, calc);
        if (!ct_equal(stored, calc, sizeof calc)) { st = DB_ERR_MAC; goto done; }
    } else if (strcmp(footer, "END") == 0) {
        if (key) { st = DB_ERR_NOMAC; goto done; }
    } else {
        goto done;
    }

    /* Разбор тела (целостность уже подтверждена). */
    buf[body_len] = '\0';
    char *line = buf, *nl;
    int first = 1;
    st = DB_OK;
    while (st == DB_OK && line < buf + body_len && (nl = strchr(line, '\n')) != NULL) {
        *nl = '\0';
        if (first) {
            if (strcmp(line, DB_MAGIC) != 0) st = DB_ERR_FORMAT;
            first = 0;
        } else {
            st = parse_line(line, out);
        }
        line = nl + 1;
    }
    if (st == DB_OK && first) st = DB_ERR_FORMAT;
    if (st == DB_OK) entrylist_sort(out);

done:
    free(buf);
    return st;
}
