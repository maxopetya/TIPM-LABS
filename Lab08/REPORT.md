# Лабораторная работа №8

**Тема:** Изучение систем автоматизации развёртывания и управления приложениями на примере Docker
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: Windows 11 + Docker Desktop 4.71 (engine 29.4.1), сборка внутри контейнера на `ubuntu:22.04` (в методичке указан `ubuntu:18.04`, EOL — apt-репозитории недоступны без правки sources.list, заменил на ближайший LTS).

## Tutorial

За основу взят проект из lab07. Содержимое скопировано в `Lab08/`, привязки к Hunter-пакету `mylog` из lab07 в `demo/main.cpp` и `CMakeLists.txt` убраны — в этой лабе акцент на докеризации, mylog внутри образа не нужен. Цель `demo` переписана так, как ожидает методичка: читает stdin построчно и пишет каждую строку в файл по пути из переменной `LOG_PATH`.

`demo/main.cpp`:

```cpp
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    const char* log_path = std::getenv("LOG_PATH");
    if (!log_path) {
        log_path = "log.txt";
    }

    std::ofstream log(log_path, std::ios::app);
    if (!log) {
        std::cerr << "demo: cannot open log file: " << log_path << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        log << line << '\n';
        log.flush();
    }
    return 0;
}
```

В `CMakeLists.txt` `hunter_add_package(GTest)` обёрнут в `if(BUILD_TESTS)`, так чтобы при сборке в Docker без тестов Hunter не лез за GTest.

`Dockerfile` собран послойно командами из методички (`cat > Dockerfile <<EOF`, потом несколько `cat >> Dockerfile <<EOF`), итог:

```dockerfile
FROM ubuntu:22.04

RUN apt update
RUN apt install -yy gcc g++ cmake

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

Сборка образа:

```sh
$ docker build -t logger .
...
 => [3/9] RUN apt update
 => [4/9] RUN apt install -yy gcc g++ cmake
 => [5/9] COPY . print/
 => [6/9] RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release ...
 => [7/9] RUN cmake --build _build
 => [8/9] RUN cmake --build _build --target install
 => exporting to image
 => naming to docker.io/library/logger:latest
```

Список образов:

```sh
$ docker images
IMAGE           ID             DISK USAGE   CONTENT SIZE
logger:latest   bd5e3c63f2ae        703MB          203MB
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

В `README.md` lab-ссылки обновлены под lab08 (`sed -i 's/lab07/lab08/g; s/lab06/lab08/g' README.md`).

Файл `.travis.yml` приведён к формату из методички:

```yaml
language: cpp

services:
  - docker

script:
  - docker build -t logger .
```

Шаги `travis login --auto` / `travis enable` пропущены — сервис Travis CI закрыт для бесплатных аккаунтов, в lab04 этот момент уже зафиксирован.

## Ссылки

- Docker Engine reference: https://docs.docker.com/engine/reference/builder/
- Docker get-started: https://docs.docker.com/get-started/
- The Docker Book: https://www.dockerbook.com

## Вывод

Сборка С++ проекта изолирована в контейнере: для повторения окружения с любой машины достаточно `docker build -t logger .` — toolchain (gcc/g++/cmake) подтягивается из базового образа, никакого ручного `apt install` на хосте не требуется. Передача состояния наружу выполняется через `VOLUME /home/logs`: процесс внутри контейнера пишет в `$LOG_PATH`, файл оказывается на хостовой ФС через bind-mount `-v`. Конфигурация приложения вынесена в `ENV` — путь к логу не зашит в исходник, его можно переопределить при запуске.
