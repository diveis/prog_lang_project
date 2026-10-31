# hashwatch

[![CI](https://github.com/D/hashwatch/actions/workflows/ci.yml/badge.svg)](https://github.com/D/hashwatch/actions)

**Консольная утилита проверки целостности файлов и обнаружения изменений по сохранённой базе контрольных сумм.**
Создаёт базу хешей каталога и сообщает об изменённых, удалённых и новых файлах.

## Возможности

* Рекурсивное хеширование каталога (SHA-256, реализация по **FIPS 180-4**, без внешних зависимостей).
* Отчёт `MODIFIED` / `DELETED` / `NEW` и итоговая сводка.
* **Защита самой базы** от подделки через HMAC-SHA256 (**RFC 2104**): `-k <ключ>`.
* Понятные коды возврата (0 / 1 / 2) — удобно для cron и CI.
* Атомарная запись базы, база может лежать внутри проверяемого каталога.
* Сборка через **Make** или **CMake**, unit- и интеграционные тесты, CI на GitHub Actions.

## Технологии

C11 (POSIX), Make / CMake, Git, FIPS 180-4 (SHA-256), RFC 2104 (HMAC).

## Быстрый старт

```sh
git clone https://github.com/<your-name>/hashwatch.git
cd prog_lang_project
make            # бинарник: build/hashwatch
make test       # запуск тестов
sudo make install   # опционально, в /usr/local/bin
```

Через CMake:

```sh
cmake -S . -B build-cmake && cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
```

## Использование

```sh
# 1. Ключ для защиты базы (по желанию) — храните отдельно от базы
head -c 32 /dev/urandom > hw.key && chmod 600 hw.key

# 2. Создать базу
hashwatch init ~/project -d project.hwdb -k hw.key

# 3. Проверять
hashwatch check ~/project -d project.hwdb -k hw.key
```

Пример вывода:

```
MODIFIED: src/main.c
NEW:      notes.txt
DELETED:  old/config.ini
Summary: 1 modified, 1 deleted, 1 new, 42 unchanged
```

Код возврата: `0` — без изменений, `1` — есть изменения, `2` — ошибка
(в том числе подделанная база или неверный ключ).

Все команды и опции — в [docs/USAGE.md](docs/USAGE.md).

## Документация

| Документ | Содержание |
|----------|------------|
| [docs/USAGE.md](docs/USAGE.md) | Руководство пользователя |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Модули, алгоритм, модель угроз |
| [docs/DB_FORMAT.md](docs/DB_FORMAT.md) | Формат файла базы |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Сборка, тесты, стиль коммитов |
| [CHANGELOG.md](CHANGELOG.md) | История версий |

## Структура репозитория

```
src/        sha256, hmac, hexutil, entry, scan, db, diff, main
tests/      unit-тесты (векторы NIST и RFC 4231) и integration.sh
docs/       документация
.github/    CI
Makefile, CMakeLists.txt
```

## Ограничения

Платформы POSIX (Linux, macOS, BSD). Отслеживается только содержимое обычных файлов
(не права, не владелец, не симлинки). Подробнее — в разделе «Модель угроз» в
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
