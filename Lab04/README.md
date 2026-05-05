# Lab04 — CI

[![CI](https://github.com/maxopetya/lab04/actions/workflows/ci.yml/badge.svg)](https://github.com/maxopetya/lab04/actions/workflows/ci.yml)
[![Build status](https://ci.appveyor.com/api/projects/status/opabdqtrx7qxiqsn?svg=true)](https://ci.appveyor.com/project/maxopetya/lab04)

Учебный репозиторий по четвёртой лабораторной работе:
непрерывная интеграция для CMake-проекта из Lab03.

Содержимое каталога:

- `sources/`, `include/`, `examples/`, `formatter_lib/`, `formatter_ex_lib/`,
  `solver_lib/`, `hello_world_application/`, `solver_application/` — проект
  Formatter Inc. из Lab03.
- `.github/workflows/ci.yml` — сборка на Linux (gcc, clang) через GitHub Actions.
- `appveyor.yml` — сборка на Windows (MSVC) через AppVeyor.

Travis CI из исходного задания заменён на GitHub Actions.
