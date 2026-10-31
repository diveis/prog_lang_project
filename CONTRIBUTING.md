# Участие в разработке

## Сборка и тесты

```sh
make            # собрать build/hashwatch
make test       # unit-тесты + интеграционные тесты
```

или через CMake:

```sh
cmake -S . -B build-cmake && cmake --build build-cmake && ctest --test-dir build-cmake --output-on-failure
```

Проверка санитайзерами:

```sh
make clean && CFLAGS="-O1 -g -fsanitize=address,undefined" make test
```

## Стиль коммитов

Используем [Conventional Commits](https://www.conventionalcommits.org/):
`feat:`, `fix:`, `docs:`, `test:`, `build:`, `ci:`, `refactor:`, `chore:`.

## Правила

* Код на C11 + POSIX.1-2008, без предупреждений при `-Wall -Wextra -Wpedantic`.
* Каждое изменение поведения сопровождается тестом.
* Изменение формата базы — только с повышением версии и обновлением `docs/DB_FORMAT.md`.
