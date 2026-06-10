# Лабораторная работа №11

**Тема:** Изучение процесса создания сеансов совместной разработки с использованием инструмента ngrok
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc 13.3.0, GNU Make 4.3. Зависимости tmux собирались из исходников в локальный префикс `~/install`.

## Tutorial

### Подготовка

```sh
cd ~
mkdir install tmp
export HOME_PREFIX=`pwd`/install
export USERNAME=`whoami`
cd tmp
```

### libevent 2.1.8

```sh
wget https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz
tar -xvzf libevent-2.1.8-stable.tar.gz
cd libevent-2.1.8-stable
./configure --prefix=${HOME_PREFIX}
make && make install
cd ..
```

В `evutil_rand.c` пришлось обернуть вызов `arc4random_addrandom` в `#ifndef EVENT__HAVE_ARC4RANDOM` — в glibc 2.36+ функции уже нет, и без правки сборка падает на линковке примера.

### ncurses 6.6

```sh
wget https://invisible-island.net/datafiles/release/ncurses.tar.gz
tar -xvzf ncurses.tar.gz
cd ncurses-6.6
./configure --prefix=${HOME_PREFIX} --with-shared --enable-pc-files \
  --with-pkg-config-libdir=${HOME_PREFIX}/lib/pkgconfig
make && make install
cd ..
```

### tmux 2.5

```sh
wget https://github.com/tmux/tmux/releases/download/2.5/tmux-2.5.tar.gz
tar -xvzf tmux-2.5.tar.gz
cd tmux-2.5
./configure --prefix=${HOME_PREFIX} \
  CFLAGS="-I${HOME_PREFIX}/include -I${HOME_PREFIX}/include/ncursesw" \
  LDFLAGS="-L${HOME_PREFIX}/lib"
make && make install
cd ..
```

`configure` искал `-lncurses`, поэтому сделал symlink `libncursesw.so → libncurses.so` (и для `.a`).

```sh
$ ~/install/bin/tmux -V
tmux 2.5
```

### ngrok

```sh
curl -sL https://ngrok-agent.s3.amazonaws.com/pool/main/n/ngrok/ngrok_3.39.2-0_amd64.deb -o ngrok.deb
dpkg-deb -x ngrok.deb ngrok-deb
cp ngrok-deb/usr/local/bin/ngrok ~/install/bin/ngrok
```

```sh
$ ~/install/bin/ngrok version
ngrok version 3.39.2
```

### tmux-сессия и SSH

```sh
export LD_LIBRARY_PATH=${HOME_PREFIX}/lib
export PATH="${HOME_PREFIX}/bin:${PATH}"
tmux new -d -s session_with_group
```

В WSL нет systemd, sshd запускал руками:

```sh
sudo apt install -y openssh-server
sudo ssh-keygen -A
sudo mkdir -p /run/sshd
sudo /usr/sbin/sshd
```

Открытый ключ положил в `~/.ssh/authorized_keys`.

### Запуск ngrok-туннеля

```sh
ngrok config add-authtoken <token>
ngrok tcp 22
```

```
ngrok                                                                          (Ctrl+C to quit)

Session Status                online
Account                       maxopetya (Plan: Free)
Version                       3.39.2
Region                        Europe (eu)
Latency                       42ms
Web Interface                 http://127.0.0.1:4040
Forwarding                    tcp://4.tcp.eu.ngrok.io:14823 -> localhost:22

Connections                   ttl     opn     rt1     rt5     p50     p90
                              0       0       0.00    0.00    0.00    0.00
```

### Подключение к общей сессии через ngrok

Из второго терминала:

```sh
$ ssh maxop@4.tcp.eu.ngrok.io -p 14823
Welcome to Ubuntu 24.04 LTS

$ tmux a -t session_with_group
```

Оба клиента в одной сессии:

```sh
$ tmux list-clients -t session_with_group
/dev/pts/0: session_with_group [80x23 xterm-256color] (utf8)
/dev/pts/2: session_with_group [80x23 xterm-256color] (utf8)
```

Команды, набранные во втором клиенте, видны в первом. У ngrok в статусе после подключения:

```
Connections                   ttl     opn     rt1     rt5     p50     p90
                              1       1       0.20    0.04    18.40   24.32
```

## Ссылки

- ngrok: https://ngrok.com/
- tmux: https://github.com/tmux/tmux
- libevent: https://libevent.org
- ncurses: https://invisible-island.net/ncurses/

## Вывод

ngrok пробрасывает локальный TCP-порт за NAT на свой публичный адрес, не требуя белого IP и проброса портов на роутере. В связке с tmux это даёт сеанс совместной разработки: один участник поднимает именованную сессию и `ngrok tcp 22`, остальные подключаются по выданному адресу через SSH и командой `tmux a` цепляются к общему терминалу. В работе собраны libevent 2.1.8, ncurses 6.6 и tmux 2.5 из исходников, поставлен ngrok 3.39.2, поднят sshd. Туннель `ngrok tcp 22` запущен, через выданный адрес выполнено внешнее SSH-подключение к общей tmux-сессии — оба клиента видят один экран.
