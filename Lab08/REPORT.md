# Лабораторная работа №8

**Тема:** Изучение систем автоматизации развёртывания и управления приложениями на примере Docker
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: Windows 11 + Docker Desktop 4.71 (engine 29.4.1), сборка внутри контейнера на `ubuntu:22.04` (в методичке указан `ubuntu:18.04`, EOL — apt-репозитории недоступны без правки sources.list, заменил на ближайший актуальный LTS).

## Tutorial

За основу взят проект из lab07 — библиотека `print`, приложение `demo`, которое читает stdin и пишет строки в файл по `LOG_PATH` (через `print(text, out)`), и `mylog_app`, использующее Hunter-пакет `mylog` 0.2.0. Содержимое скопировано в `Lab08/`, добавлен submodule `tools/polly` (для конкретно этой лабы не используется, но идёт с проектом lab07).

`demo/main.cpp` (без изменений из lab07):

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

`Dockerfile` собран по шагам из методички (`cat > Dockerfile`, потом несколько `cat >> Dockerfile`). Поскольку `CMakeLists.txt` тянет через Hunter пакеты `GTest` (с пином версии в `cmake/Hunter/config.cmake`) и `mylog` (нет в основном индексе Hunter, регистрация лежит только в локальном клоне), в образ кладётся клон cpp-pm/hunter и mylog-конфиг — без этого `cmake -H. -B_build` падает на `find_package(mylog ...)`.

`Dockerfile`:

```dockerfile
FROM ubuntu:22.04

RUN apt update
RUN apt install -yy gcc g++ cmake git

RUN git clone --depth 1 https://github.com/cpp-pm/hunter.git /opt/hunter
COPY .docker/mylog-hunter.cmake /opt/hunter/cmake/projects/mylog/hunter.cmake
RUN sed -i '/hunter_default_version(mxnet/a hunter_default_version(mylog VERSION 0.2.0)' \
    /opt/hunter/cmake/configs/default.cmake
ENV HUNTER_ROOT=/opt/hunter

COPY . print/
WORKDIR print

RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=_install
RUN cmake --build _build
RUN cmake --build _build --target install

ENV LOG_PATH /home/logs/log.txt

VOLUME /home/logs

WORKDIR _install/bin

ENTRYPOINT ./demo
```

`.docker/mylog-hunter.cmake` — копия `cmake/projects/mylog/hunter.cmake` из локального клона Hunter:

```cmake
hunter_add_version(
    PACKAGE_NAME mylog
    VERSION "0.2.0"
    URL "https://github.com/maxopetya/mylog/archive/v0.2.0.tar.gz"
    SHA1 46009ec73c2881734ea6c49ba54e998d8c15e9dc
)
...
hunter_pick_scheme(DEFAULT url_sha1_cmake)
hunter_cacheable(mylog)
hunter_download(PACKAGE_NAME mylog)
```

Также добавлен `.dockerignore`, чтобы не тащить в build-context лишнее (`.git`, `tools/polly/`, `logs/`, `_build/`, `_install/`).

Сборка образа:

```sh
$ docker build -t logger .
...
 => [3/12] RUN apt update
 => [4/12] RUN apt install -yy gcc g++ cmake git
 => [5/12] RUN git clone --depth 1 https://github.com/cpp-pm/hunter.git /opt/hunter
 => [6/12] COPY .docker/mylog-hunter.cmake /opt/hunter/cmake/projects/mylog/hunter.cmake
 => [7/12] RUN sed -i '/hunter_default_version(mxnet/a hunter_default_version(mylog VERSION 0.2.0)' ...
 => [9/12] COPY . print/
 => [10/12] RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=_install
 => [11/12] RUN cmake --build _build
 => [12/12] RUN cmake --build _build --target install
 => exporting to image ... naming to docker.io/library/logger:latest
```

В процессе Hunter скачал и собрал `GTest` 1.14 и `mylog` 0.2.0 в `/opt/hunter/_Base/...`, после чего обычный cmake-билд собрал `demo`, `mylog_app`, `print`, `formatter`, `formatter_ex`, `solver_lib`, `hello_world`, `solver`, `banking` и поставил `demo` + `mylog_app` в `/print/_install/bin/`.

Список образов:

```sh
$ docker images
IMAGE           ID             DISK USAGE   CONTENT SIZE
logger:latest   307a6358b403        818MB          228MB
```

Запуск с пробросом тома и тремя строками на stdin:

```sh
$ mkdir logs
$ docker run -it -v "$(pwd)/logs/:/home/logs/" logger
text1
text2
text3
<C-D>
```

`docker inspect logger` (вырезка по существу):

```json
"Config": {
    "Env": [
        "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        "HUNTER_ROOT=/opt/hunter",
        "LOG_PATH=/home/logs/log.txt"
    ],
    "Entrypoint": [
        "/bin/sh", "-c", "./demo"
    ],
    "Volumes": {
        "/home/logs": {}
    },
    "WorkingDir": "/print/_install/bin"
}
```

Содержимое лога после прогона:

```sh
$ cat logs/log.txt
text1
text2
text3
```

Проверил заодно второй бинарь `mylog_app` (он не запускается через ENTRYPOINT по умолчанию, нужно явно):

```sh
$ docker run --rm --entrypoint /print/_install/bin/mylog_app -v "$(pwd)/logs:/home/logs" logger
$ cat logs/log.txt
[2026-05-19 23:02:27] [INFO] application started
[2026-05-19 23:02:27] [WARN] disk space is low
[2026-05-19 23:02:27] [ERROR] network unreachable
```

Hunter-пакет `mylog` 0.2.0 (libmylog.a + заголовки + cmake-конфиг) внутри образа собран и подцеплен, всё работает.

В `README.md` обновлены ссылки на лабу под lab08 (`sed -i 's/lab07/lab08/g; s/lab06/lab08/g' README.md`).

Файл `.travis.yml`:

```yaml
language: cpp

services:
  - docker

script:
  - docker build -t logger .
```

Шаги `travis login --auto` / `travis enable` не выполнял — Travis CI для бесплатных аккаунтов закрыт (фиксировалось ещё в lab04).

## Ссылки

- Docker Engine reference: https://docs.docker.com/engine/reference/builder/
- Docker get-started: https://docs.docker.com/get-started/
- The Docker Book: https://www.dockerbook.com
- Hunter (cpp-pm): https://hunter.readthedocs.io/

## Вывод

Сборка С++ проекта из lab07 изолирована в Docker-образе: для повторения окружения с любой машины достаточно `docker build -t logger .` — toolchain (gcc/g++/cmake/git) подтягивается из базового образа, локальный клон Hunter с регистрацией кастомного пакета `mylog` тоже кладётся в образ на этапе сборки, никакого ручного `apt install` или подготовки `~/.hunter` на хосте не требуется. Передача состояния наружу выполняется через `VOLUME /home/logs`: `demo` пишет читаемые из stdin строки в `$LOG_PATH`, файл оказывается на хостовой ФС через bind-mount `-v`. Конфигурация приложения вынесена в `ENV` — путь к логу не зашит в исходник, его можно переопределить при `docker run`.
