# Лабораторная работа №9

**Тема:** Изучение сервисов хранения артефактов на примере GitHub Release
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: WSL2 Ubuntu 24.04, gcc 13.3.0, cmake 3.28.3, GnuPG 2.4.4, github-release v0.10.0, Hunter (cpp-pm) с локальным кэшем mylog 0.2.0 из lab07.

## Tutorial

За основу взят проект из lab08 (библиотека `print`, приложение `demo`, `mylog_app` на Hunter-пакете `mylog`, Dockerfile). Содержимое скопировано в `~/tipm-work/lab09`, git инициализирован с нуля, remote выставлен на новый публичный репозиторий `maxopetya/lab09`.

```sh
cp -r ~/tipm-work/TIPM-LABS/Lab08/. ~/tipm-work/lab09/
cd ~/tipm-work/lab09
rm -rf _build _install logs
git init -b master
git remote add origin https://github.com/maxopetya/lab09.git
```

Методичка предлагает клонировать `lab08` как отдельный репозиторий — в этом курсе lab08 у меня в TIPM-LABS, отдельного `maxopetya/lab08` нет, поэтому копирую файлами и инициализирую заново. Все `lab08 → lab09` в `README.md` заменяет `sed`, в этой ветке проекта в README не было упоминаний `lab08` (он остался с тематикой CPack из lab06), правок не потребовалось.

### GPG-ключ

В WSL генерирую ed25519-ключ батч-режимом (без интерактивных подсказок и без passphrase — иначе подпись тега требует ввод пароля при каждом действии gpg):

```sh
cat > /tmp/gpg-batch <<EOF
%no-protection
Key-Type: EDDSA
Key-Curve: ed25519
Key-Usage: sign,cert
Subkey-Type: ECDH
Subkey-Curve: cv25519
Subkey-Usage: encrypt
Name-Real: maxopetya
Name-Email: maxopetya@proton.me
Expire-Date: 0
%commit
EOF
gpg --batch --generate-key /tmp/gpg-batch
```

```sh
$ gpg --list-secret-keys --keyid-format LONG
sec   ed25519/83ACC5E6454AE4D4 2026-05-19 [SC]
      9E4D84DF0C6289C30BC9DE7283ACC5E6454AE4D4
uid                 [ultimate] maxopetya <maxopetya@proton.me>
ssb   cv25519/FF0FD10C22BF9484 2026-05-19 [E]
```

ASCII-armored публичная часть экспортирована и загружена на GitHub через `gh gpg-key add` (предварительно расширил scope токена: `gh auth refresh -s write:gpg_key,admin:gpg_key`). Сразу проверил:

```sh
$ gh gpg-key list
maxopetya@proton.me  83ACC5E6454AE4D4  ...  2026-05-20T02:31:32+03:00
```

Шаги с `xclip` / `pbcopy` / `pbpaste` из методички не использовал — WSL headless, буфера обмена нет. Ключ перенёс через файл (`gpg --armor --export ... > /tmp/pubkey.asc` → `gh gpg-key add /tmp/pubkey.asc`).

Настройка git на подпись:

```sh
git config user.signingkey 83ACC5E6454AE4D4
git config gpg.program gpg
echo 'export GPG_TTY=$(tty)' >> ~/.profile
```

### Сборка пакета

`CMakeLists.txt` уже подключает `CPackConfig.cmake` (наследие lab06). Конфигурация и сборка с TGZ-генератором:

```sh
export HUNTER_ROOT=$HOME/tipm-work/hunter
cmake -H. -B_build -DCPACK_GENERATOR="TGZ" -DCMAKE_BUILD_TYPE=Release
cmake --build _build -j4
cmake --build _build --target package
```

```
CPack: Create package using TGZ
CPack: Install projects
CPack: - Install project: print []
CPack: Create package
CPack: - package: /home/maxop/tipm-work/lab09/_build/print-0.1.0.0-Linux.tar.gz generated.
```

Артефакт `print-0.1.0.0-Linux.tar.gz`, 12835 байт. Содержимое:

```
print-0.1.0.0-Linux/cmake/print-config.cmake
print-0.1.0.0-Linux/cmake/print-config-release.cmake
print-0.1.0.0-Linux/include/print.hpp
print-0.1.0.0-Linux/lib/libprint.a
print-0.1.0.0-Linux/bin/demo
print-0.1.0.0-Linux/bin/mylog_app
```

### Подпись и пуш тега

Первый коммит и пуш в `master`:

```sh
git add -A
git commit -m "init"
git push -u origin master
```

```
To https://github.com/maxopetya/lab09.git
 * [new branch]      master -> master
```

Создание подписанного тега и проверка подписи:

```sh
$ git tag -s -m "v0.1.0.0" v0.1.0.0
$ git tag -v v0.1.0.0
gpg: Signature made Wed May 20 02:32:42 2026 MSK
gpg:                using EDDSA key 9E4D84DF0C6289C30BC9DE7283ACC5E6454AE4D4
gpg: Good signature from "maxopetya <maxopetya@proton.me>" [ultimate]
object 1e5bd101c11236369eae24e0d08ec95155e7807d
type commit
tag v0.1.0.0
tagger maxopetya <maxopetya@proton.me> 1779233562 +0300
```

```sh
git push origin master --tags
```

```
To https://github.com/maxopetya/lab09.git
 * [new tag]         v0.1.0.0 -> v0.1.0.0
```

Шаг `travis login --auto && travis enable` пропустил — Travis CI для бесплатных аккаунтов закрыт, фиксировалось ещё в lab04 / lab08.

### Релиз и загрузка артефакта

Бинарник `github-release` (v0.10.0, проект `github-release/github-release` — форк aktau из методички) положил в `~/.local/bin`:

```sh
cd /tmp
wget -q https://github.com/github-release/github-release/releases/download/v0.10.0/linux-amd64-github-release.bz2
bzip2 -d linux-amd64-github-release.bz2
chmod +x linux-amd64-github-release
mv linux-amd64-github-release ~/.local/bin/github-release
```

Go тулчейн не ставил (методичка предлагает `go get github.com/aktau/github-release`) — взял готовый релизный бинарь.

`GITHUB_TOKEN` — токен из `gh auth token`. Создание релиза:

```sh
$ export GITHUB_TOKEN=...
$ github-release release \
    --user maxopetya \
    --repo lab09 \
    --tag v0.1.0.0 \
    --name "libprint" \
    --description "my first release"
```

Загрузка артефакта с именем по `uname`:

```sh
$ export PACKAGE_OS=$(uname -s) PACKAGE_ARCH=$(uname -m)
$ export PACKAGE_FILENAME=print-${PACKAGE_OS}-${PACKAGE_ARCH}.tar.gz
$ cp _build/print-0.1.0.0-Linux.tar.gz _build/${PACKAGE_FILENAME}
$ github-release upload \
    --user maxopetya \
    --repo lab09 \
    --tag v0.1.0.0 \
    --name "${PACKAGE_FILENAME}" \
    --file _build/${PACKAGE_FILENAME}
```

Проверка через `github-release info`:

```
tags:
- v0.1.0.0 (commit: https://api.github.com/repos/maxopetya/lab09/commits/1e5bd101c11236369eae24e0d08ec95155e7807d)
releases:
- v0.1.0.0, name: 'libprint', description: 'my first release', id: 325459197, tagged: 19/05/2026 at 23:32, published: 19/05/2026 at 23:33, draft: ✗, prerelease: ✗
  - artifact: print-Linux-x86_64.tar.gz, downloads: 0, state: uploaded, type: application/octet-stream, size: 13 kB, id: 424619025
```

Скачивание и распаковка с чистой стороны:

```sh
$ mkdir -p /tmp/lab09-check && cd /tmp/lab09-check
$ wget https://github.com/maxopetya/lab09/releases/download/v0.1.0.0/print-Linux-x86_64.tar.gz
2026-05-20 02:35:57 (2.01 MB/s) - 'print-Linux-x86_64.tar.gz' saved [12835/12835]
$ tar -ztf print-Linux-x86_64.tar.gz
print-0.1.0.0-Linux/cmake/print-config.cmake
print-0.1.0.0-Linux/cmake/print-config-release.cmake
print-0.1.0.0-Linux/include/print.hpp
print-0.1.0.0-Linux/lib/libprint.a
print-0.1.0.0-Linux/bin/demo
print-0.1.0.0-Linux/bin/mylog_app
```

Размер совпадает с локально собранным TGZ, артефакт целый.

## Ссылки

- Репозиторий лабы: https://github.com/maxopetya/lab09
- Релиз: https://github.com/maxopetya/lab09/releases/tag/v0.1.0.0
- github-release: https://github.com/github-release/github-release
- Creating Releases: https://help.github.com/articles/creating-releases/
- Signing Commits: https://help.github.com/articles/signing-commits-with-gpg/
- Creating a personal access token: https://help.github.com/articles/creating-a-personal-access-token-for-the-command-line/

## Вывод

GitHub Release связывает три вещи в один публичный артефакт: git-тег с подписью владельца (`v0.1.0.0` подписан ed25519-ключом `83ACC5E6454AE4D4`, `git tag -v` показывает `Good signature`), бинарный пакет, собранный CPack-ом из CMake-проекта (`print-Linux-x86_64.tar.gz`), и метаданные релиза (название, описание, дата). Залить артефакт стороннему пользователю — `wget` по прямой ссылке, ни клонировать репозиторий, ни собирать его не требуется. Подпись тега даёт GitHub-у возможность пометить релиз как пришедший именно от владельца аккаунта, а не от того, у кого случайно оказался push-доступ. Для автоматизации (CI) удобно использовать `github-release` CLI: он принимает токен из `$GITHUB_TOKEN` и заливает файлы одним вызовом, в отличие от веб-формы релизов.
