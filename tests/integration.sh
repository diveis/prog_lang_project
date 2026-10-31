#!/bin/sh
# Интеграционные тесты CLI. Использование: sh tests/integration.sh <path/to/hashwatch>
BIN=${1:-build/hashwatch}
case "$BIN" in /*) ;; *) BIN="$PWD/$BIN" ;; esac

T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
FAILED=0

# expect_rc <ожидаемый код> <описание> <команда...>
expect_rc() {
    want=$1; name=$2; shift 2
    "$@" >"$T/out" 2>"$T/err"
    got=$?
    if [ "$got" -ne "$want" ]; then
        echo "FAIL: $name (rc=$got, expected $want)"; cat "$T/out" "$T/err"
        FAILED=1
    else
        echo "ok:   $name"
    fi
}
expect_out() {   # expect_out <regex> <описание>
    if grep -q "$1" "$T/out" "$T/err"; then echo "ok:   $2"
    else echo "FAIL: $2 (no match for '$1')"; cat "$T/out" "$T/err"; FAILED=1; fi
}

D="$T/data"; DB="$T/db.hwdb"; KEY="$T/k.key"
mkdir -p "$D/sub dir"
echo one   > "$D/a.txt"
echo two   > "$D/sub dir/b file.txt"
echo "super-secret-key" > "$KEY"

# --- без HMAC ---
expect_rc 0 "init"                      "$BIN" init "$D" -d "$DB"
expect_rc 2 "init refuses overwrite"    "$BIN" init "$D" -d "$DB"
expect_rc 0 "init -f overwrites"        "$BIN" init "$D" -d "$DB" -f
expect_rc 0 "check clean"               "$BIN" check "$D" -d "$DB"
expect_out "0 modified, 0 deleted, 0 new, 2 unchanged" "summary for clean state"

echo changed >> "$D/a.txt"
rm "$D/sub dir/b file.txt"
echo fresh > "$D/c.txt"
expect_rc 1 "check detects changes"     "$BIN" check "$D" -d "$DB"
expect_out "^MODIFIED: a.txt"           "reports modified"
expect_out "^DELETED:  sub dir/b file.txt" "reports deleted (path with spaces)"
expect_out "^NEW:      c.txt"           "reports new"

expect_rc 1 "update reports changes"    "$BIN" update "$D" -d "$DB"
expect_rc 0 "check clean after update"  "$BIN" check "$D" -d "$DB"

# база внутри проверяемого каталога не считается файлом
DB2="$D/inside.hwdb"
expect_rc 0 "init with db inside dir"   "$BIN" init "$D" -d "$DB2"
expect_rc 0 "check with db inside dir"  "$BIN" check "$D" -d "$DB2"

# --- с HMAC ---
rm -f "$DB2"
expect_rc 0 "init with key"             "$BIN" init "$D" -d "$DB" -k "$KEY" -f
expect_rc 0 "check with key"            "$BIN" check "$D" -d "$DB" -k "$KEY"
expect_rc 2 "check without key fails"   "$BIN" check "$D" -d "$DB"
echo "wrong" > "$T/bad.key"
expect_rc 2 "check with wrong key fails" "$BIN" check "$D" -d "$DB" -k "$T/bad.key"
expect_out "HMAC mismatch"              "wrong key message"

# подмена базы: меняем хеш одного файла - MAC должен не совпасть
sed '2s/^./0/' "$DB" > "$T/forged.hwdb"
expect_rc 2 "forged database rejected"  "$BIN" check "$D" -d "$T/forged.hwdb" -k "$KEY"

# подмена: убрали MAC, подсунули базу без защиты
expect_rc 0 "init unprotected db"       "$BIN" init "$D" -d "$T/plain.hwdb"
expect_rc 2 "downgrade (no MAC) rejected" "$BIN" check "$D" -d "$T/plain.hwdb" -k "$KEY"

# --- ошибки использования ---
expect_rc 2 "no arguments"              "$BIN"
expect_rc 2 "unknown command"           "$BIN" frobnicate "$D" -d "$DB"
expect_rc 2 "missing directory"         "$BIN" check "$T/nope" -d "$DB" -k "$KEY"
expect_rc 2 "missing database"          "$BIN" check "$D" -d "$T/none.hwdb"
expect_rc 0 "--help"                    "$BIN" --help

exit $FAILED
