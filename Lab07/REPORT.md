# Лабораторная работа №7

**Тема:** Изучение систем управления пакетами на примере Hunter
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 19.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc 13.3.0, cmake 3.28.3, Hunter (cpp-pm) v0.23.251 + локальный клон HEAD.

## Tutorial

За базу взят проект из lab06 (библиотека `print` + `banking` + тесты на gtest/gmock из `third-party/gtest`). Цель — заменить вендоринг gtest на пакет, подтягиваемый Hunter.

Подключение HunterGate:

```sh
cd ~/tipm-work/lab07
mkdir -p cmake
wget https://raw.githubusercontent.com/cpp-pm/gate/master/cmake/HunterGate.cmake \
    -O cmake/HunterGate.cmake
```

В начало `CMakeLists.txt` добавлен блок:

```cmake
cmake_minimum_required(VERSION 3.4)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.251.tar.gz"
    SHA1 "5659b15dc0884d4b03dbd95710e6a1fa0fc3258d"
    LOCAL
)
```

Папка `third-party/gtest` удалена через `git rm -rf`. Вместо неё:

```cmake
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
```

Цель `check` теперь линкуется с `GTest::gmock_main` вместо вендоренного `gmock_main`.

Первая попытка сборки с дефолтной версией GTest из Hunter (1.10.0) упала: gcc 13 включает `-Werror=maybe-uninitialized`, и `gtest-death-test.cc` не собирается. По методичке решается через свой `cmake/Hunter/config.cmake` и локальный клон Hunter:

```sh
git clone https://github.com/cpp-pm/hunter ~/tipm-work/hunter
export HUNTER_ROOT=$HOME/tipm-work/hunter
```

```cmake
# cmake/Hunter/config.cmake
hunter_config(GTest VERSION 1.14.0)
```

В HunterGate добавлен флаг `LOCAL`, стандарт повышен до C++14 (требование GTest 1.13+). Пересборка:

```sh
rm -rf _builds
cmake -H. -B_builds -DBUILD_TESTS=ON
cmake --build _builds
cmake --build _builds --target test
```

Тест прошёл:

```
1/1 Test #1: check ............................   Passed    0.00 sec
100% tests passed, 0 tests failed out of 1
```

Hunter сложил скачанное в `~/.hunter` (≈35 МБ — gtest со всеми артефактами кэшируется и переиспользуется между проектами).

## Homework

**Задание:** создать свой Hunter-пакет.

Сделан минимальный header-only логгер `mylog` (две функции: `info`, `warn`).

Содержимое пакета (`mylog-pkg/`):

```cpp
// include/mylog/mylog.hpp
#pragma once
#include <iostream>
#include <string>

namespace mylog {
inline void info(const std::string& msg) { std::cout << "[info] " << msg << std::endl; }
inline void warn(const std::string& msg) { std::cerr << "[warn] " << msg << std::endl; }
}
```

```cmake
# CMakeLists.txt пакета
cmake_minimum_required(VERSION 3.5)
project(mylog VERSION 0.1.0 LANGUAGES CXX)

add_library(mylog INTERFACE)
target_include_directories(mylog INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS mylog EXPORT mylog-config)
install(EXPORT mylog-config NAMESPACE mylog:: DESTINATION lib/cmake/mylog)
```

Пакет залит в отдельный публичный репозиторий и помечен тегом:

```sh
git init -b master && git add -A && git commit -m "init"
git remote add origin https://github.com/maxopetya/mylog.git
git push -u origin master
git tag v0.1.0 && git push origin v0.1.0
```

SHA1 архива релиза:

```sh
$ wget https://github.com/maxopetya/mylog/archive/v0.1.0.tar.gz -O /tmp/mylog.tar.gz
$ sha1sum /tmp/mylog.tar.gz
d0f1575878b29ce39046b06f285f35d22b2c5b0f  /tmp/mylog.tar.gz
```

Регистрация в локальном клоне Hunter — `cmake/projects/mylog/hunter.cmake`:

```cmake
include(hunter_add_version)
include(hunter_cacheable)
include(hunter_cmake_args)
include(hunter_download)
include(hunter_pick_scheme)

hunter_add_version(
    PACKAGE_NAME mylog
    VERSION "0.1.0"
    URL "https://github.com/maxopetya/mylog/archive/v0.1.0.tar.gz"
    SHA1 d0f1575878b29ce39046b06f285f35d22b2c5b0f
)

hunter_cmake_args(mylog CMAKE_ARGS CMAKE_BUILD_TYPE=Release)
hunter_pick_scheme(DEFAULT url_sha1_cmake)
hunter_cacheable(mylog)
hunter_download(PACKAGE_NAME mylog)
```

В `cmake/configs/default.cmake` добавлена запись (строго в алфавитном порядке, между `mxnet` и `nanoflann` — Hunter проверяет порядок):

```cmake
hunter_default_version(mylog VERSION 0.1.0)
```

Использование в самом lab07 — мини-приложение `demo`:

```cpp
// demo/main.cpp
#include <mylog/mylog.hpp>

int main() {
    mylog::info("hunter package works");
    mylog::warn("this is a warning");
    return 0;
}
```

В `CMakeLists.txt` рядом с GTest:

```cmake
hunter_add_package(mylog)
find_package(mylog CONFIG REQUIRED)

add_executable(demo ${CMAKE_CURRENT_SOURCE_DIR}/demo/main.cpp)
target_link_libraries(demo mylog::mylog)
install(TARGETS demo RUNTIME DESTINATION bin)
```

Сборка и запуск:

```sh
$ cmake --build _builds
$ ./_builds/demo
[info] hunter package works
[warn] this is a warning
```

Hunter скачал архив `mylog` с GitHub, распаковал, сконфигурировал, установил в `~/.hunter/_Base/.../Install` и подключил через `find_package`. Никакого вендоринга — пакет используется как внешняя зависимость наравне с GTest.

## Ссылки

- Репозиторий пакета: https://github.com/maxopetya/mylog
- Hunter: https://hunter.readthedocs.io/
- Create Hunter package: https://docs.hunter.sh/en/latest/creating-new/create.html
- Custom Hunter config: https://github.com/cpp-pm/hunter/wiki/example.custom.config.id

## Вывод

Замена вендоренного gtest на пакет из Hunter сократила репозиторий на ~200 файлов и убрала ручное обновление третьесторонних зависимостей — версия пакета задаётся одной строкой в `cmake/Hunter/config.cmake`. Создание собственного Hunter-пакета `mylog` показало полный путь: написать CMake-конфиг с правильным install/export, выложить релиз в публичный репозиторий, прописать URL и SHA1 в локальном клоне Hunter, после чего пакет подключается через стандартные `hunter_add_package` и `find_package` — точно так же, как любой пакет из основного индекса.
