# Лабораторная работа №3

**Тема:** Изучение систем автоматизации сборки проекта на примере CMake

**Студент:** Приходько Максим Максимович, ИУ8-24

**Дата:** 05.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc/g++ 13.3.0, GNU Make 4.3, cmake 3.28.3 (бинарник от Kitware, распакован в `~/tools` без sudo).

## Tutorial

### Подготовка

В качестве отправной точки взят `Lab02/` из этого же репозитория — оттуда переехали `sources/print.cpp`, `include/print.hpp` и `examples/example{1,2}.cpp`. `cmake` в WSL не установлен из apt (требует sudo), поэтому скачан официальный портативный бинарник:

```sh
$ curl -sL -o cmake.tar.gz \
    https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.tar.gz
$ tar xzf cmake.tar.gz -C ~/tools
$ ln -s ~/tools/cmake-3.28.3-linux-x86_64/bin/cmake ~/bin/cmake
$ cmake --version
cmake version 3.28.3
```

### Сборка вручную (без CMake)

```sh
$ g++ -std=c++11 -I./include -c sources/print.cpp
$ nm print.o | grep print
0000000000000000 T _Z5printRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERSo
000000000000002a T _Z5printRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERSt14basic_ofstreamIcS2_E
$ ar rvs print.a print.o
$ file print.a
print.a: current ar archive
$ g++ -std=c++11 -I./include -c examples/example1.cpp
$ g++ example1.o print.a -o example1
$ ./example1 && echo
hello
$ g++ -std=c++11 -I./include -c examples/example2.cpp
$ g++ example2.o print.a -o example2
$ ./example2
$ cat log.txt && echo
hello
```

После проверки артефакты удалены (`rm -rf *.o *.a example1 example2 log.txt`).

### Сборка через CMake (инкрементально)

`CMakeLists.txt` собирался по шагам, как в инструкции. Финальная промежуточная версия:

```cmake
cmake_minimum_required(VERSION 3.4)
project(print)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(print STATIC ${CMAKE_CURRENT_SOURCE_DIR}/sources/print.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(example1 ${CMAKE_CURRENT_SOURCE_DIR}/examples/example1.cpp)
add_executable(example2 ${CMAKE_CURRENT_SOURCE_DIR}/examples/example2.cpp)

target_link_libraries(example1 print)
target_link_libraries(example2 print)
```

Конфигурация и сборка:

```sh
$ cmake -H. -B_build
-- The C compiler identification is GNU 13.3.0
-- The CXX compiler identification is GNU 13.3.0
...
-- Configuring done (1.6s)
-- Generating done (0.0s)
-- Build files have been written to: .../lab03/_build
$ cmake --build _build
[ 33%] Built target print
[ 66%] Built target example1
[100%] Built target example2
$ _build/example1 && echo
hello
$ _build/example2 && cat log.txt && echo
hello
```

`cmake 3.28.3` ругается на `cmake_minimum_required(VERSION 3.4)` (deprecated <3.5), но сборку не валит — формулировка взята из инструкции, оставлена как есть.

### Финальный CMakeLists.txt с install

Версия из апстрима `tp-labs/lab03` подменяет ручной — добавляет опцию `BUILD_EXAMPLES`, `target_include_directories` с генератор-выражениями, install-таргеты для библиотеки, заголовков и cmake-конфига. Конфигурация с префиксом и установка:

```sh
$ cmake -H. -B_build -DCMAKE_INSTALL_PREFIX=_install
$ cmake --build _build --target install
[100%] Built target print
Install the project...
-- Install configuration: ""
-- Installing: .../_install/lib/libprint.a
-- Installing: .../_install/include
-- Installing: .../_install/include/print.hpp
-- Installing: .../_install/cmake/print-config.cmake
-- Installing: .../_install/cmake/print-config-noconfig.cmake
```

Содержимое `_install` (вместо `tree` использован `find`, т.к. `tree` в WSL не установлен):

```
_install/
  cmake/
    print-config-noconfig.cmake
    print-config.cmake
  include/
    print.hpp
  lib/
    libprint.a
```

## Homework

Все три задания решены в одном проекте: к корневому `CMakeLists.txt` (из tutorial-а) добавляются `add_subdirectory(...)` для каждой новой компоненты. Иерархия зависимостей:

```
hello_world ──▶ formatter_ex ──▶ formatter
solver ─────▶ formatter_ex ──▶ formatter
       └────▶ solver_lib
```

### Part I — formatter_lib

Файлы `formatter_lib/formatter.{h,cpp}` положены в каталог. `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.4)
project(formatter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(formatter STATIC ${CMAKE_CURRENT_SOURCE_DIR}/formatter.cpp)

target_include_directories(formatter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

`PUBLIC` у `target_include_directories` — чтобы потребители (formatter_ex) подхватывали заголовок без явного указания путей.

### Part II — formatter_ex_lib

`formatter_ex_lib/formatter_ex.{h,cpp}` использует `formatter.h`. Линковка с `formatter` транзитивно прокидывает include-директорию:

```cmake
cmake_minimum_required(VERSION 3.4)
project(formatter_ex)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(formatter_ex STATIC ${CMAKE_CURRENT_SOURCE_DIR}/formatter_ex.cpp)

target_include_directories(formatter_ex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(formatter_ex formatter)
```

### Part III — hello_world и solver

`hello_world_application/CMakeLists.txt`:

```cmake
add_executable(hello_world ${CMAKE_CURRENT_SOURCE_DIR}/hello_world.cpp)
target_link_libraries(hello_world formatter_ex)
```

`solver_lib` — обёртка над квадратным уравнением. Исходник из апстрима использует `std::sqrtf`, который в `<cmath>` gcc 13 / libstdc++ под именем `std::` не виден. Заменено на `std::sqrt(d)` (есть перегрузка `float` с C++98), также добавлен `#include <cmath>` (в апстриме его нет).

```cmake
add_library(solver_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/solver.cpp)
target_include_directories(solver_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

`solver_application/CMakeLists.txt`:

```cmake
add_executable(solver ${CMAKE_CURRENT_SOURCE_DIR}/equation.cpp)
target_link_libraries(solver formatter_ex solver_lib)
```

В корневой `CMakeLists.txt` добавлены пять строк:

```cmake
add_subdirectory(formatter_lib)
add_subdirectory(formatter_ex_lib)
add_subdirectory(solver_lib)
add_subdirectory(hello_world_application)
add_subdirectory(solver_application)
```

### Полная сборка и проверка

```sh
$ cmake -H. -B_build -DCMAKE_INSTALL_PREFIX=_install -DBUILD_EXAMPLES=ON
$ cmake --build _build
[ 12%] Built target print
[ 25%] Built target example1
[ 37%] Built target example2
[ 50%] Built target formatter
[ 62%] Built target formatter_ex
[ 75%] Built target solver_lib
[ 87%] Built target hello_world
[100%] Built target solver

$ _build/hello_world_application/hello_world
-------------------------
hello, world!
-------------------------

$ echo "1 -3 2" | _build/solver_application/solver
-------------------------
x1 = 1.000000
-------------------------
-------------------------
x2 = 2.000000
-------------------------

$ echo "1 0 1" | _build/solver_application/solver
-------------------------
error: discriminant < 0
-------------------------
```

`x² - 3x + 2` корректно даёт `x1=1, x2=2`; `x² + 1` ловит ветку с отрицательным дискриминантом и через `std::logic_error` уходит в форматирование ошибки.

`cmake --build _build --target install` ставит только `print`-часть (для homework-таргетов install-правил в задании не требовалось):

```
_install/
  bin/
    example1
    example2
  cmake/
    print-config-noconfig.cmake
    print-config.cmake
  include/
    print.hpp
  lib/
    libprint.a
```

## История коммитов

```
049fde8 equation
bc6cde4 hello world
212b8c0 solver
d6f551d formatter_ex
94c190e formatter
87ccc42 tutorial
231c5e0 init
```

## Ссылки

- Репозиторий: <https://github.com/maxopetya/TIPM-LABS>
- Каталог лабы: <https://github.com/maxopetya/TIPM-LABS/tree/master/Lab03>
- Задание: <https://github.com/tp-labs/lab03>

## Вывод

Освоен инкрементальный путь сборки: ручные `g++/ar` → плоский `CMakeLists.txt` → апстримный с install-правилами. По домашке поднята иерархия из двух статических библиотек (`formatter`, `formatter_ex`), вспомогательной `solver_lib` и двух исполняемых таргетов (`hello_world`, `solver`); линковка на нужные библиотеки разруливается через `target_link_libraries`, заголовки прокидываются `target_include_directories(... PUBLIC ...)`. Всё собирается одной командой `cmake --build _build` из корня `Lab03/`.
