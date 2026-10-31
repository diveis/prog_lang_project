# Changelog

Формат — [Keep a Changelog](https://keepachangelog.com/ru/1.1.0/), версии — [SemVer](https://semver.org/lang/ru/).

## [0.1.0] - 2026-10-31

### Added
- SHA-256 по FIPS 180-4 (проверено векторами NIST).
- HMAC-SHA256 по RFC 2104 (проверено векторами RFC 4231).
- Рекурсивный сканер каталога, исключение файла базы из учёта.
- Текстовая база хешей версии 1 с опциональной HMAC-защитой.
- Команды `init`, `check`, `update`; коды возврата 0/1/2.
- Сборка через Make и CMake, unit- и интеграционные тесты, GitHub Actions.
- Документация: README, USAGE, ARCHITECTURE, DB_FORMAT, CONTRIBUTING.
