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
