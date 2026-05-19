# Лабораторная работа №7

**Тема:** Изучение систем управления пакетами на примере Hunter
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc 13.3.0, cmake 3.28.3, Hunter (cpp-pm) v0.23.251 + локальный клон HEAD.

## Tutorial

За базу взят проект из lab06 (библиотека `print` + `banking` + тесты на gtest/gmock из `third-party/gtest`).

Подключение HunterGate:

```sh
cd ~/tipm-work/lab07
mkdir -p cmake
wget https://raw.githubusercontent.com/cpp-pm/gate/master/cmake/HunterGate.cmake \
    -O cmake/HunterGate.cmake
```

В начало `CMakeLists.txt`:

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

Первая сборка с дефолтной версией GTest из Hunter (1.10.0) упала: gcc 13 включает `-Werror=maybe-uninitialized`, и `gtest-death-test.cc` не собирается. По методичке решается через свой `cmake/Hunter/config.cmake` и локальный клон Hunter:

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

Hunter сложил скачанное в `~/.hunter` (≈35 МБ — gtest со всеми артефактами кэшируется).

### demo с print и LOG_PATH

По методичке в проект добавлено мини-приложение `demo/main.cpp`, которое использует библиотеку `print` и пишет читаемые из stdin строки в файл, путь к которому задан переменной окружения `LOG_PATH`:

```cpp
#include <print.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  const char* log_path = std::getenv("LOG_PATH");
  if (log_path == nullptr) {
    std::cerr << "undefined environment variable: LOG_PATH" << std::endl;
    return 1;
  }
  std::string text;
  while (std::cin >> text) {
    std::ofstream out{log_path, std::ios_base::app};
    print(text, out);
    out << std::endl;
  }
}
```

В `CMakeLists.txt`:

```cmake
add_executable(demo ${CMAKE_CURRENT_SOURCE_DIR}/demo/main.cpp)
target_link_libraries(demo print)
install(TARGETS demo RUNTIME DESTINATION bin)
```

Запуск:

```sh
$ echo -e "one\ntwo\nthree" | LOG_PATH=/tmp/lab07.log ./_builds/demo
$ cat /tmp/lab07.log
one
two
three
```

### Polly

Polly подключён как toolchain-wrapper:

```sh
mkdir tools
git clone https://github.com/ruslo/polly tools/polly
```

Сборка проекта через Polly с дефолтным toolchain:

```sh
$ python3 tools/polly/bin/polly.py --toolchain default --reconfig --jobs 4
...
[100%] Built target banking
Log saved: /home/maxop/tipm-work/lab07/_logs/polly/default/log.txt
Generate: 0:00:11s
Build: 0:00:03s
Total: 0:00:14s
SUCCESS
```

Polly прозрачно прокидывает HunterGate, поднимает все зависимости, собирает все таргеты проекта одной командой и пишет лог в `_logs/polly/<toolchain>/log.txt`.

## Homework

**Задание:** создать свой Hunter-пакет.

Сделана отдельная C++ библиотека-логгер `mylog` с уровнями `INFO`/`WARN`/`ERROR` и метками времени. Структура пакета:

```
mylog/
├── CMakeLists.txt
├── README.md
├── include/mylog/mylog.hpp
└── src/mylog.cpp
```

Заголовок `include/mylog/mylog.hpp`:

```cpp
#pragma once
#include <iosfwd>
#include <string>

namespace mylog {

enum class Level { Info, Warn, Error };

const char* to_string(Level level);

class Logger {
 public:
  explicit Logger(std::ostream& out);
  void log(Level level, const std::string& message);
  void info(const std::string& message);
  void warn(const std::string& message);
  void error(const std::string& message);
 private:
  std::ostream& out_;
};

}
```

Реализация в `src/mylog.cpp` форматирует сообщения вида
`[2026-05-20 01:46:20] [INFO] message` и пишет их в переданный `std::ostream`.

`CMakeLists.txt` пакета:

```cmake
cmake_minimum_required(VERSION 3.5)
project(mylog VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mylog STATIC src/mylog.cpp)

target_include_directories(mylog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS mylog
    EXPORT mylog-config
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(EXPORT mylog-config NAMESPACE mylog:: DESTINATION lib/cmake/mylog)
```

Пакет залит в публичный репозиторий и помечен тегом релиза:

```sh
git push -u origin master
git tag v0.2.0 && git push origin v0.2.0
```

SHA1 архива релиза:

```sh
$ wget https://github.com/maxopetya/mylog/archive/v0.2.0.tar.gz -O /tmp/mylog.tar.gz
$ sha1sum /tmp/mylog.tar.gz
46009ec73c2881734ea6c49ba54e998d8c15e9dc  /tmp/mylog.tar.gz
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
    VERSION "0.2.0"
    URL "https://github.com/maxopetya/mylog/archive/v0.2.0.tar.gz"
    SHA1 46009ec73c2881734ea6c49ba54e998d8c15e9dc
)

hunter_cmake_args(mylog CMAKE_ARGS CMAKE_BUILD_TYPE=Release)
hunter_pick_scheme(DEFAULT url_sha1_cmake)
hunter_cacheable(mylog)
hunter_download(PACKAGE_NAME mylog)
```

В `cmake/configs/default.cmake` добавлена запись (строго в алфавитном порядке, между `mxnet` и `nanoflann` — Hunter проверяет порядок):

```cmake
hunter_default_version(mylog VERSION 0.2.0)
```

Использование в самом lab07 — приложение `mylog_app/`:

```cpp
#include <mylog/mylog.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
  const char* log_path = std::getenv("LOG_PATH");
  if (log_path != nullptr) {
    std::ofstream file{log_path, std::ios_base::app};
    mylog::Logger logger(file);
    logger.info("application started");
    logger.warn("disk space is low");
    logger.error("network unreachable");
    return 0;
  }
  mylog::Logger logger(std::cout);
  logger.info("application started");
  logger.warn("disk space is low");
  logger.error("network unreachable");
  return 0;
}
```

В `CMakeLists.txt`:

```cmake
hunter_add_package(mylog)
find_package(mylog CONFIG REQUIRED)

add_executable(mylog_app ${CMAKE_CURRENT_SOURCE_DIR}/mylog_app/main.cpp)
target_link_libraries(mylog_app mylog::mylog)
install(TARGETS mylog_app RUNTIME DESTINATION bin)
```

Сборка и запуск:

```sh
$ cmake --build _builds
$ ./_builds/mylog_app
[2026-05-20 01:46:20] [INFO] application started
[2026-05-20 01:46:20] [WARN] disk space is low
[2026-05-20 01:46:20] [ERROR] network unreachable
```

Hunter скачал архив `mylog` с GitHub, собрал статическую библиотеку `libmylog.a`, установил её вместе с заголовками и cmake-конфигом в `~/.hunter/_Base/.../Install` и подключил через стандартный `find_package(mylog CONFIG REQUIRED)`.

## Ссылки

- Репозиторий пакета: https://github.com/maxopetya/mylog
- Hunter: https://hunter.readthedocs.io/
- Create Hunter package: https://docs.hunter.sh/en/latest/creating-new/create.html
- Custom Hunter config: https://github.com/cpp-pm/hunter/wiki/example.custom.config.id
- Polly: https://github.com/ruslo/polly

## Вывод

Замена вендоренного gtest на пакет из Hunter сократила репозиторий на ~200 файлов и убрала ручное обновление третьесторонних зависимостей — версия пакета задаётся одной строкой в `cmake/Hunter/config.cmake`. Создание собственного Hunter-пакета `mylog` показало полный путь: написать библиотеку с install/export, выложить релиз в публичный репозиторий, прописать URL и SHA1 в локальном клоне Hunter, после чего пакет подключается через стандартные `hunter_add_package` и `find_package` наравне с любым другим пакетом из индекса. Polly даёт удобную обёртку над cmake для прогонов под разные toolchain-ы.
