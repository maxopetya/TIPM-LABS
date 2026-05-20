# Лабораторная работа №11

**Тема:** Изучение процесса создания сеансов совместной разработки с использованием инструмента ngrok
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc 13.3.0, GNU Make 4.3. Все библиотеки и утилиты собирались из исходников в локальный префикс `~/install`.

## Tutorial

### Подготовка окружения

Каталоги и переменные из задания:

```sh
cd ~
mkdir install tmp
export HOME_PREFIX=`pwd`/install
export USERNAME=`whoami`
cd tmp
```

`HOME_PREFIX` указывает на `/home/maxop/install` — общий префикс установки для libevent, ncurses, tmux и ngrok. Сборка ведётся в `~/tmp`.

### libevent 2.1.8

```sh
wget https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz
tar -xvzf libevent-2.1.8-stable.tar.gz
cd libevent-2.1.8-stable
./configure --prefix=${HOME_PREFIX}
make && make install
```

`make` обрывался на сборке примера `sample/dns-example`:

```sh
/usr/bin/ld: ./.libs/libevent.so: undefined reference to `arc4random_addrandom'
```

Функция `arc4random_addrandom` в glibc 2.36+ отсутствует, а libevent 2.1.8 вызывает её безусловно в `evutil_rand.c`. Вызов обёрнут в условие — при наличии системного `arc4random` он пропускается:

```c
void
evutil_secure_rng_add_bytes(const char *buf, size_t n)
{
#ifndef EVENT__HAVE_ARC4RANDOM
	arc4random_addrandom((unsigned char*)buf,
	    n>(size_t)INT_MAX ? INT_MAX : (int)n);
#else
	(void)buf; (void)n;
#endif
}
```

После правки библиотека собралась и установилась полностью, неразрешённых символов в `libevent.so` не осталось:

```sh
$ nm -D ~/install/lib/libevent.so | grep arc4random
                 U arc4random@GLIBC_2.36
                 U arc4random_buf@GLIBC_2.36
```

### ncurses

```sh
wget https://invisible-island.net/datafiles/release/ncurses.tar.gz
tar -xvzf ncurses.tar.gz
cd ncurses-6.6
./configure --prefix=${HOME_PREFIX} --with-shared --without-ada \
  --enable-pc-files --with-pkg-config-libdir=${HOME_PREFIX}/lib/pkgconfig
make && make install
```

Ссылка из задания отдаёт текущую версию — это ncurses 6.6, а не 5.9. Сборка с `--with-shared` даёт широкосимвольный вариант `libncursesw`; заголовки устанавливаются в `~/install/include/ncursesw/`. Флаг `--enable-pc-files` добавлен, чтобы установить `ncursesw.pc` — он нужен дальше для сборки tmux.

### tmux 2.5

```sh
wget https://github.com/tmux/tmux/releases/download/2.5/tmux-2.5.tar.gz
tar -xvzf tmux-2.5.tar.gz
cd tmux-2.5
./configure --prefix=${HOME_PREFIX} \
  CFLAGS="-I${HOME_PREFIX}/include -I${HOME_PREFIX}/include/ncursesw" \
  LDFLAGS="-L${HOME_PREFIX}/lib"
make && make install
```

В пути заголовков добавлен `include/ncursesw` — туда ncurses кладёт `curses.h`. `configure` всё равно не находил библиотеку curses: `pkg-config` в окружении не установлен, поэтому поиск шёл перебором `-lncurses`/`-lcurses`, а собран был только `libncursesw`. Создан symlink:

```sh
ln -s libncursesw.so ~/install/lib/libncurses.so
ln -s libncursesw.a  ~/install/lib/libncurses.a
```

После этого `configure` прошёл (`setupterm in -lncurses... yes`), tmux собрался и установился:

```sh
$ ~/install/bin/tmux -V
tmux 2.5
```

### ngrok

Ссылка из задания (`https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-amd64.zip`) ведёт на устаревший ngrok v2; загрузка с `bin.equinox.io` из текущего региона обрывается на первых килобайтах. Агент поставлен из официального apt-зеркала ngrok на Amazon S3 — пакет `.deb` распакован без установки в систему:

```sh
curl -sL https://ngrok-agent.s3.amazonaws.com/pool/main/n/ngrok/ngrok_3.39.2-0_amd64.deb -o ngrok.deb
dpkg-deb -x ngrok.deb ngrok-deb
cp ngrok-deb/usr/local/bin/ngrok ~/install/bin/ngrok
```

```sh
$ ~/install/bin/ngrok version
ngrok version 3.39.2
```

Контрольная сумма `.deb` совпала с указанной в индексе репозитория (`sha256: 3b4649fb…bc44`).

### Запуск tmux и сеанс совместной разработки

```sh
export LD_LIBRARY_PATH=${HOME_PREFIX}/lib
export PATH="${HOME_PREFIX}/bin:${PATH}"
tmux new -s session_with_group
```

`LD_LIBRARY_PATH` нужен, чтобы tmux находил собранные `libevent` и `libncursesw` в `~/install/lib`. Создаётся именованная сессия `session_with_group`; внутри неё панели делятся сочетанием `Ctrl-B "` (горизонтальный сплит). Проверка сессии с разбиением на панели:

```sh
$ tmux new -d -s session_with_group
$ tmux split-window -h -t session_with_group
$ tmux list-panes -t session_with_group
0: [40x23] [history 0/2000, 0 bytes] %0
1: [39x23] [history 0/2000, 0 bytes] %1 (active)
$ tmux ls
session_with_group: 1 windows (created Wed May 20 17:35:17 2026) [80x23]
```

Шаг `rm -rf tmp install` из задания не выполнялся — он удалил бы собранные библиотеки и бинарники, на которые опираются tmux и ngrok.

### Подключение к общей сессии по SSH

Цель туннеля — SSH-порт. В окружении не было SSH-сервера, поэтому он установлен и запущен вручную (в WSL нет systemd, демон запускается напрямую):

```sh
sudo apt install -y openssh-server
sudo ssh-keygen -A
sudo mkdir -p /run/sshd
sudo /usr/sbin/sshd
```

Для входа сгенерирована пара ключей, открытый ключ добавлен в `authorized_keys`. Подключение к собственной машине по SSH демонстрирует механику совместной сессии: участник, вошедший по SSH, попадает в ту же tmux-сессию `session_with_group`, что и хозяин:

```sh
$ ss -tlnp | grep ':22 '
LISTEN 0  4096  0.0.0.0:22  0.0.0.0:*
LISTEN 0  4096     [::]:22     [::]:*

$ tmux ls                              # через ssh-подключение
session_with_group: 1 windows (created Wed May 20 17:47:11 2026) [80x23]

$ tmux a -t session_with_group         # содержимое общей сессии
maxop@WhiteKnight:~$ echo Alisa: started shared session
Alisa: started shared session
```

Второй клиент, подключившийся по SSH, пишет в общую сессию — изменение сразу видно у первого участника:

```sh
maxop@WhiteKnight:~$ echo Alisa: started shared session
Alisa: started shared session
maxop@WhiteKnight:~$ echo Bob: connected over ssh
Bob: connected over ssh
```

### Проброс порта через ngrok

В реальном сценарии SSH-порт пробрасывается наружу через ngrok, чтобы к сессии можно было подключиться из другой сети:

```sh
# участник, открывающий доступ
ngrok authtoken ${NGROK_TOKEN}
ngrok tcp 22

# участник, подключающийся к сессии
ssh ${USERNAME}@0.tcp.ngrok.io -p<порт>
tmux a -t session_with_group
```

Туннель `ngrok tcp 22` требует верифицированного аккаунта ngrok и установленного authtoken — без него агент подключается к сервису, но получает отказ:

```sh
$ ngrok tcp 22
authentication failed: Usage of ngrok requires a verified account and authtoken.
ERR_NGROK_4018
```

Аккаунт ngrok не использовался, поэтому реальный туннель не открывался; команда `ngrok authtoken` и `ngrok tcp 22` приведены как порядок действий участника, пробрасывающего порт.

## Ссылки

- ngrok: https://ngrok.com/
- tmux: https://github.com/tmux/tmux
- libevent: https://libevent.org
- ncurses: https://invisible-island.net/ncurses/

## Вывод

ngrok создаёт защищённый туннель от публичного адреса своего сервиса к локальному порту машины за NAT, не требуя проброса портов на роутере и белого IP. В связке с tmux это даёт простой сеанс совместной разработки: один участник поднимает именованную tmux-сессию и пробрасывает наружу SSH-порт командой `ngrok tcp 22`, остальные подключаются по SSH к выданному ngrok адресу и командой `tmux a` присоединяются к общему терминалу — все видят один экран и могут работать в нём одновременно. В работе из исходников собраны зависимости tmux (libevent 2.1.8, ncurses 6.6) и сам tmux 2.5, установлен агент ngrok 3.39.2, поднят SSH-сервер. Механика общей сессии проверена: участник, вошедший по SSH, присоединяется к той же сессии `session_with_group` и его команды видны остальным. Открытие реального ngrok-туннеля не выполнялось: ngrok v3 требует верифицированного аккаунта и authtoken.
