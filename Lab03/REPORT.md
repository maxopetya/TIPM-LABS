# Лабораторная работа №3

**Тема:** Изучение систем автоматизации сборки проекта на примере CMake

**Студент:** Приходько Максим Максимович, ИУ8-24

**Дата:** 05.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc/g++ 13.3.0, GNU Make 4.3, cmake 3.28.3.

## Tutorial

### Подготовка

В качестве отправной точки взят `Lab02/` из этого же репозитория — оттуда перенесены `sources/print.cpp`, `include/print.hpp` и `examples/example{1,2}.cpp`. Установлен cmake:

```sh
$ sudo apt install -y cmake
$ cmake --version | head -1
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

После проверки промежуточные файлы удалены (`rm -rf *.o *.a example1 example2 log.txt`).

### Сборка через CMake

`CMakeLists.txt` собирался по шагам в соответствии с инструкцией. Промежуточная версия:

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

При конфигурации cmake 3.28 выводит предупреждение о том, что `cmake_minimum_required(VERSION 3.4)` соответствует устаревшей политике совместимости (deprecated <3.5). Сборка завершается успешно. Значение `3.4` сохранено в соответствии с инструкцией.

### Финальный CMakeLists.txt с установкой

Версия из репозитория `tp-labs/lab03` заменяет промежуточный файл и добавляет: опцию `BUILD_EXAMPLES`, `target_include_directories` с генератор-выражениями, install-правила для библиотеки, заголовков и cmake-конфига.

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

Содержимое каталога `_install`:

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

Все три задания выполнены в составе одного проекта: к корневому `CMakeLists.txt` добавляются `add_subdirectory(...)` для каждой компоненты. Иерархия зависимостей:

```
hello_world ──▶ formatter_ex ──▶ formatter
solver ─────▶ formatter_ex ──▶ formatter
       └────▶ solver_lib
```

### Задание 1 — formatter_lib

Файлы `formatter_lib/formatter.{h,cpp}` размещены в каталоге библиотеки. `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.4)
project(formatter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(formatter STATIC ${CMAKE_CURRENT_SOURCE_DIR}/formatter.cpp)

target_include_directories(formatter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

Уровень `PUBLIC` у `target_include_directories` обеспечивает передачу include-директории зависимым целям (formatter_ex).

### Задание 2 — formatter_ex_lib

Библиотека `formatter_ex_lib/formatter_ex.{h,cpp}` использует заголовок `formatter.h`. Зависимость от `formatter` указывается через `target_link_libraries`, что также транзитивно подключает include-директорию.

```cmake
cmake_minimum_required(VERSION 3.4)
project(formatter_ex)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(formatter_ex STATIC ${CMAKE_CURRENT_SOURCE_DIR}/formatter_ex.cpp)

target_include_directories(formatter_ex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(formatter_ex formatter)
```

### Задание 3 — hello_world и solver

`hello_world_application/CMakeLists.txt`:

```cmake
add_executable(hello_world ${CMAKE_CURRENT_SOURCE_DIR}/hello_world.cpp)
target_link_libraries(hello_world formatter_ex)
```

`solver_lib` содержит реализацию решателя квадратного уравнения. Исходный текст из задания использует `std::sqrtf`, который в стандартной библиотеке gcc 13 / libstdc++ в пространстве имён `std` отсутствует. Функция заменена на `std::sqrt(d)` (имеет перегрузку для `float` начиная с C++98); также добавлен `#include <cmath>` (в исходном файле он отсутствует).

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

Уравнение `x² - 3x + 2` даёт корни `x1 = 1`, `x2 = 2`. Для `x² + 1` дискриминант отрицательный, выбрасывается `std::logic_error` и сообщение об ошибке передаётся в форматирование.

Команда `cmake --build _build --target install` устанавливает только цели из tutorial-части (для домашних заданий install-правил по условию задания не предусмотрено):

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

## Ссылки

- Репозиторий: <https://github.com/maxopetya/TIPM-LABS>
- Каталог лабораторной работы: <https://github.com/maxopetya/TIPM-LABS/tree/master/Lab03>
- Задание: <https://github.com/tp-labs/lab03>

## Вывод

Рассмотрены три способа сборки проекта: ручной (`g++` и `ar`), через простой `CMakeLists.txt` и через финальный вариант с install-правилами. В рамках домашнего задания построена иерархия из двух статических библиотек (`formatter`, `formatter_ex`), дополнительной библиотеки `solver_lib` и двух исполняемых файлов (`hello_world`, `solver`). Зависимости между целями описаны через `target_link_libraries`, передача include-директорий — через `target_include_directories(... PUBLIC ...)`. Полная сборка проекта выполняется одной командой `cmake --build _build` из корня `Lab03/`.
