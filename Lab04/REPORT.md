# Лабораторная работа №4

**Тема:** Системы непрерывной интеграции

**Студент:** Приходько Максим Максимович, ИУ8-24

**Дата:** 05.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc 13.3.0, cmake 3.28.3, GitHub CLI 2.92.0.
Удалённые сборочные агенты: `ubuntu-latest` (GitHub Actions, cmake 4.x,
gcc/clang) и `Visual Studio 2022` (AppVeyor, cmake 4.1.0, MSVC x64).

Travis CI из исходного задания заменён на GitHub Actions: `travis-ci.org`
закрыт, бесплатные кредиты `travis-ci.com` для open-source ограничены.

## Tutorial

### Подготовка репозитория

Создан публичный репозиторий `maxopetya/lab04`:

```sh
$ gh repo create maxopetya/lab04 --public \
      --description "Лабораторная работа №4 — непрерывная интеграция (GitHub Actions + AppVeyor)"
https://github.com/maxopetya/lab04
```

В рабочую копию скопировано содержимое `lab03` (мульти-модульный
CMake-проект Formatter Inc.: библиотеки `print`, `formatter`, `formatter_ex`,
`solver_lib` и приложения `hello_world`, `solver`):

```sh
$ git clone https://github.com/maxopetya/TIPM-LABS.git /tmp/lab04-work
$ mkdir /tmp/lab04-pub && cp -r /tmp/lab04-work/Lab03/. /tmp/lab04-pub/
$ rm /tmp/lab04-pub/REPORT.md
```

Локальная сборка в WSL для проверки исходного состояния:

```sh
$ cmake -H. -B_build -DCMAKE_INSTALL_PREFIX=_install
-- Configuring done (2.1s)
-- Generating done (0.0s)
-- Build files have been written to: /tmp/lab04-build/_build
$ cmake --build _build
[100%] Built target solver
$ cmake --build _build --target install
-- Installing: /tmp/lab04-build/_install/lib/libprint.a
-- Installing: /tmp/lab04-build/_install/include/print.hpp
-- Installing: /tmp/lab04-build/_install/cmake/print-config.cmake
```

### Конфигурация GitHub Actions

`.github/workflows/ci.yml` — сборка на `ubuntu-latest`, матрица из двух
компиляторов:

```yaml
name: CI
on:
  push: { branches: [ master, main ] }
  pull_request: { branches: [ master, main ] }
jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        compiler:
          - { name: gcc,   cc: gcc,   cxx: g++ }
          - { name: clang, cc: clang, cxx: clang++ }
    name: linux-${{ matrix.compiler.name }}
    env:
      CC: ${{ matrix.compiler.cc }}
      CXX: ${{ matrix.compiler.cxx }}
    steps:
      - uses: actions/checkout@v4
      - run: cmake --version && ${CC} --version && ${CXX} --version
      - run: cmake -H. -B_build -DCMAKE_INSTALL_PREFIX=_install -DCMAKE_POLICY_VERSION_MINIMUM=3.5
      - run: cmake --build _build
      - run: cmake --build _build --target install
```

Бейдж в README:

```markdown
[![CI](https://github.com/maxopetya/lab04/actions/workflows/ci.yml/badge.svg)](https://github.com/maxopetya/lab04/actions/workflows/ci.yml)
```

### Push и первый прогон

```sh
$ git init -b master
$ git add . && git commit -m "init"
$ git remote add origin https://github.com/maxopetya/lab04.git
$ git push -u origin master
```

Обе сборки `linux-gcc` и `linux-clang` зелёные:

```sh
$ gh api repos/maxopetya/lab04/commits/master/check-runs \
      --jq '.check_runs[] | {name, conclusion}'
{"conclusion":"success","name":"linux-clang"}
{"conclusion":"success","name":"linux-gcc"}
```

## Homework

В исходном задании Homework требует настроить сборку библиотек и приложений
из lab03 на двух платформах: Linux (gcc и clang) и Windows. Linux-часть
закрыта в Tutorial матрицей GitHub Actions; для Windows подключён AppVeyor.

### Подключение AppVeyor

На сервисе [ci.appveyor.com](https://ci.appveyor.com) выполнен вход через
GitHub OAuth. В `NEW PROJECT → GitHub` добавлен репозиторий
`maxopetya/lab04`. В разделе `Settings → Badges` получен фрагмент
markdown-бейджа.

### Конфигурация AppVeyor

`appveyor.yml`:

```yaml
version: 1.0.{build}
image: Visual Studio 2022
platform: [ x64 ]
configuration: [ Release ]
clone_folder: c:\projects\lab04

install:
  - cmake --version

build: off

build_script:
  - cmd: cmake -H. -B_build -A x64 -DCMAKE_INSTALL_PREFIX=_install -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  - cmd: cmake --build _build --config %CONFIGURATION%
  - cmd: cmake --build _build --config %CONFIGURATION% --target install

test: off

artifacts:
  - path: _install
    name: lab04-install
```

Бейдж добавлен в README рядом с бейджем GitHub Actions:

```markdown
[![Build status](https://ci.appveyor.com/api/projects/status/opabdqtrx7qxiqsn?svg=true)](https://ci.appveyor.com/project/maxopetya/lab04)
```

### Прогоны и правки конфигурации

#### Первый прогон

AppVeyor — `failed`:

```
[00:00:02] The build phase is set to "MSBuild" mode (default), but no
Visual Studio project or solution files were found in the root directory.
If you are not building Visual Studio project switch build mode to
"Script" and provide your custom build command.
```

В `appveyor.yml` добавлено `build: off` — отключение MSBuild-режима
по умолчанию, чтобы AppVeyor не искал `*.sln` в корне и сразу выполнял
`build_script`.

#### Второй прогон

AppVeyor — снова `failed`, на этапе `cmake -H. -B_build`:

```
[00:00:02] cmake version 4.1.0
[00:00:02] -- Building for: Visual Studio 17 2022
[00:00:02] CMake Error at CMakeLists.txt:1 (cmake_minimum_required):
[00:00:02]   Compatibility with CMake < 3.5 has been removed from CMake.
[00:00:02]   Or, add -DCMAKE_POLICY_VERSION_MINIMUM=3.5 to try
[00:00:02]   configuring anyway.
[00:00:02] -- Configuring incomplete, errors occurred!
```

В исходном `CMakeLists.txt` из lab03 объявлено
`cmake_minimum_required(VERSION 3.4)`, что в CMake 4.x приводит к ошибке.
Альтернатива — поднять минимум во всех `CMakeLists.txt` подкаталогов,
но это меняло бы исходник lab03. В команды конфигурации обоих CI
добавлен флаг `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.

#### Третий прогон

Все три сборки зелёные:

```sh
$ gh api repos/maxopetya/lab04/commits/master/status \
      --jq '{state, statuses: [.statuses[] | {context, state}]}'
{
  "state": "success",
  "statuses": [
    { "context": "continuous-integration/appveyor/branch", "state": "success" }
  ]
}
```

| Сервис         | Платформа | Компиляторы    | Конфиг                     | Результат |
|----------------|-----------|----------------|----------------------------|-----------|
| GitHub Actions | Linux     | gcc, clang     | `.github/workflows/ci.yml` | success   |
| AppVeyor       | Windows   | MSVC (VS 2022) | `appveyor.yml`             | success   |

Артефакт сборки на Windows публикуется как `lab04-install` (содержимое
`_install/`: статические библиотеки, заголовки, CMake-конфиг пакета
`print`, `hello_world.exe`, `solver.exe`).

## Ссылки

- Репозиторий лабы: <https://github.com/maxopetya/lab04>
- GitHub Actions: <https://github.com/maxopetya/lab04/actions>
- AppVeyor: <https://ci.appveyor.com/project/maxopetya/lab04>
- Каталог в учебном репозитории: <https://github.com/maxopetya/TIPM-LABS/tree/master/Lab04>

## Вывод

Для проекта lab03 настроена непрерывная интеграция на двух платформах:
GitHub Actions (Linux, gcc + clang) и AppVeyor (Windows, MSVC). Все три
сборки проходят успешно, бейджи добавлены в README репозитория lab04.
