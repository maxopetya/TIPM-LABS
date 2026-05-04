# Лабораторная работа №2

**Тема:** Изучение системы контроля версий Git

**Студент:** Приходько Максим Максимович, ИУ8-24

**Дата:** 05.05.2026

Окружение: WSL2 Ubuntu 24.04, git 2.43.0, clang-format 18.1.3, GitHub CLI 2.92.0.

Работа выполнена в репозитории курса
[`maxopetya/TIPM-LABS`](https://github.com/maxopetya/TIPM-LABS) в подкаталоге
`Lab02/`. Отдельный репозиторий `lab02` не создавался; команды tutorial-а
адаптированы под существующий репозиторий, последовательность действий и
артефакты соответствуют инструкции.

## Tutorial

### Подготовка окружения

```sh
$ export GITHUB_USERNAME=maxopetya
$ export GITHUB_EMAIL=maxopetya@proton.me
$ git config --global user.name  ${GITHUB_USERNAME}
$ git config --global user.email ${GITHUB_EMAIL}
$ git config --global init.defaultBranch master
```

Авторизация в GitHub выполнена через `gh auth login`, токен сохранён в
git-credential-store:

```sh
$ gh auth status
github.com
  ✓ Logged in to github.com account maxopetya
  - Token scopes: 'gist', 'read:org', 'repo', 'workflow'
$ git config --global credential.helper "store --file=$HOME/.git-credentials"
```

### Клонирование и инициализация

Репозиторий `TIPM-LABS` уже создан публичным; он был склонирован пустым:

```sh
$ mkdir -p ~/lab02-work && cd ~/lab02-work
$ git clone https://github.com/maxopetya/TIPM-LABS.git
Cloning into 'TIPM-LABS'...
warning: You appear to have cloned an empty repository.
$ cd TIPM-LABS
$ git checkout -b master
```

Создан корневой `README.md`, выполнен первый коммит и push:

```sh
$ git add README.md
$ git commit -m "init: TIPM-LABS root README"
$ git push -u origin master
```

### Структура учебного материала

В каталоге `Lab02/` созданы файлы по образцу tutorial-а:

```sh
$ mkdir -p Lab02/sources Lab02/include Lab02/examples
$ cat > Lab02/.gitignore <<'EOF'
*build*/
*install*/
*.swp
.idea/
EOF
```

Содержимое `Lab02/include/print.hpp`:

```cpp
#include <fstream>
#include <iostream>
#include <string>

void print(const std::string& text, std::ofstream& out);
void print(const std::string& text, std::ostream& out = std::cout);
```

Содержимое `Lab02/sources/print.cpp`:

```cpp
#include <print.hpp>

void print(const std::string& text, std::ostream& out)
{
  out << text;
}

void print(const std::string& text, std::ofstream& out)
{
  out << text;
}
```

Файлы `Lab02/examples/example1.cpp` и `Lab02/examples/example2.cpp` приведены
по образцу инструкции и отдельно не дублируются в отчёте.

```sh
$ git add Lab02/
$ git commit -m "Lab02: tutorial structure (sources, include, examples)"
$ git push origin master
```

## Homework

### Part I

Программа `Lab02/hello_world.cpp` создана в «плохом стиле» с директивой
`using namespace std;` после заголовочных файлов:

```cpp
#include <iostream>
using namespace std;

int main() {
    cout << "Hello world" << endl;
    return 0;
}
```

```sh
$ git add Lab02/hello_world.cpp
$ git commit -m "Lab02: added hello_world (homework Part I, step 3-5)"
```

Затем код изменён так, чтобы имя пользователя считывалось из стандартного
потока ввода:

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string name;
    cout << "Enter your name: ";
    getline(cin, name);
    cout << "Hello world from " << name << endl;
    return 0;
}
```

Проверка сборки и запуска:

```sh
$ g++ -std=c++17 Lab02/hello_world.cpp -o /tmp/hw && echo Maxim | /tmp/hw
Enter your name: Hello world from Maxim
```

Коммит и push:

```sh
$ git commit -am "Lab02: hello_world reads user name from stdin (Part I, step 6-7)"
$ git push origin master
```

**Шаг 7 — почему не нужен повторный `git add`.** Файл уже находится под
версионным контролем (был добавлен в индекс на шаге 4 и закоммичен).
Команда `git commit -a` автоматически переносит в индекс изменения уже
отслеживаемых файлов; явный `git add` нужен только для новых либо
ранее не отслеживавшихся файлов.

История коммитов проверена в удалённом репозитории через
`gh repo view --web` и доступна на странице репозитория.

### Part II

Создана локальная ветка `lab02-patch1` от `master`:

```sh
$ git checkout -b lab02-patch1
Switched to a new branch 'lab02-patch1'
```

Из `hello_world.cpp` удалена директива `using namespace std;`, имена из
пространства `std` квалифицированы явно:

```cpp
#include <iostream>
#include <string>

int main() {
    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    std::cout << "Hello world from " << name << std::endl;
    return 0;
}
```

```sh
$ git commit -am "Lab02: drop using-namespace-std (Part II)"
$ git push -u origin lab02-patch1
```

Создан pull-request `lab02-patch1 -> master`:

```sh
$ gh pr create --base master --head lab02-patch1 \
      --title "Lab02 patch1: drop using namespace std" \
      --body "..."
https://github.com/maxopetya/TIPM-LABS/pull/1
```

В ту же ветку добавлены комментарии в исходный код, ещё один коммит и push:

```sh
$ git commit -am "Lab02: add inline comments (Part II, step 6-7)"
$ git push origin lab02-patch1
```

В созданном pull-request видны оба коммита:

```sh
$ gh pr view 1 --json commits --jq '.commits[].messageHeadline'
Lab02: drop using-namespace-std (Part II)
Lab02: add inline comments (Part II, step 6-7)
```

Слияние pull-request и удаление удалённой ветки:

```sh
$ gh pr merge 1 --merge --delete-branch
```

Локальная синхронизация и удаление локальной копии ветки:

```sh
$ git checkout master
$ git pull origin master
Updating e9f21f9..ea74cb4
Fast-forward
 Lab02/hello_world.cpp | 12 +++++++-----
$ git fetch --prune
$ git branch -d lab02-patch1
Deleted branch lab02-patch1 (was 20f5452).
```

Фрагмент истории `master` после слияния:

```
*   ea74cb4 Merge pull request #1 from maxopetya/lab02-patch1
|\
| * 20f5452 Lab02: add inline comments (Part II, step 6-7)
| * 268f790 Lab02: drop using-namespace-std (Part II)
|/
* e9f21f9 Lab02: hello_world reads user name from stdin (Part I, step 6-7)
```

### Part III

Создана локальная ветка `lab02-patch2` от обновлённого `master`:

```sh
$ git checkout -b lab02-patch2
```

К файлу применён clang-format со стилем Mozilla:

```sh
$ clang-format -i -style=Mozilla Lab02/hello_world.cpp
```

Изменилось форматирование (отступ 2 пробела, открывающая скобка функции на
отдельной строке, тип возвращаемого значения на отдельной строке):

```cpp
int
main()
{
  std::string name;
  // Запрашиваем имя у пользователя.
  std::cout << "Enter your name: ";
  std::getline(std::cin, name);
  // Печатаем приветствие в стандартный поток вывода.
  std::cout << "Hello world from " << name << std::endl;
  return 0;
}
```

```sh
$ git commit -am "Lab02: apply clang-format -style=Mozilla (Part III)"
$ git push -u origin lab02-patch2
$ gh pr create --base master --head lab02-patch2 \
      --title "Lab02 patch2: apply clang-format -style=Mozilla" --body "..."
https://github.com/maxopetya/TIPM-LABS/pull/2
```

Параллельно в `master` отредактированы комментарии — уточнены формулировки
(коммит `aa98b57`, push в `origin/master`). После этого pull-request
помечен GitHub как конфликтный:

```sh
$ gh pr view 2 --json mergeable,mergeStateStatus
{"mergeStateStatus":"DIRTY","mergeable":"CONFLICTING"}
```

Локально выполнено перебазирование ветки `lab02-patch2` на актуальный
`master`:

```sh
$ git checkout lab02-patch2
$ git fetch origin
$ git rebase origin/master
Auto-merging Lab02/hello_world.cpp
CONFLICT (content): Merge conflict in Lab02/hello_world.cpp
error: could not apply 6d7b539... Lab02: apply clang-format -style=Mozilla (Part III)
```

Конфликт в `Lab02/hello_world.cpp` разрешён вручную: сохранены новые
комментарии из `master` и форматирование Mozilla из ветки `lab02-patch2`.
После проверки сборки rebase продолжен:

```sh
$ g++ -std=c++17 Lab02/hello_world.cpp -o /tmp/hw && echo Maxim | /tmp/hw
Enter your name: Hello world from Maxim
$ git add Lab02/hello_world.cpp
$ GIT_EDITOR=true git rebase --continue
[detached HEAD 1b1c2b0] Lab02: apply clang-format -style=Mozilla (Part III)
Successfully rebased and updated refs/heads/lab02-patch2.
```

Force-push в удалённую ветку:

```sh
$ git push --force-with-lease origin lab02-patch2
 + 6d7b539...1b1c2b0 lab02-patch2 -> lab02-patch2 (forced update)
```

После force-push pull-request стал чистым и был смержен:

```sh
$ gh pr view 2 --json mergeable,mergeStateStatus
{"mergeStateStatus":"CLEAN","mergeable":"MERGEABLE"}
$ gh pr merge 2 --merge --delete-branch
```

Финальная история `master`:

```
*   4e58bc4 Merge pull request #2 from maxopetya/lab02-patch2
|\
| * 1b1c2b0 Lab02: apply clang-format -style=Mozilla (Part III)
|/
* aa98b57 Lab02: refine comments in hello_world (Part III, conflict trigger)
*   ea74cb4 Merge pull request #1 from maxopetya/lab02-patch1
|\
| * 20f5452 Lab02: add inline comments (Part II, step 6-7)
| * 268f790 Lab02: drop using-namespace-std (Part II)
|/
* e9f21f9 Lab02: hello_world reads user name from stdin (Part I, step 6-7)
* 9f24329 Lab02: added hello_world (homework Part I, step 3-5)
* 9960e77 Lab02: tutorial structure (sources, include, examples)
* eaf3139 init: TIPM-LABS root README
```

## Ссылки

- Репозиторий: <https://github.com/maxopetya/TIPM-LABS>
- PR #1 (Part II): <https://github.com/maxopetya/TIPM-LABS/pull/1>
- PR #2 (Part III): <https://github.com/maxopetya/TIPM-LABS/pull/2>

## Вывод

В рамках работы пройден типовой цикл командной разработки в Git: создание
репозитория и первичных коммитов, работа в feature-ветках, оформление и
слияние pull-request-ов, разрешение конфликта при перебазировании и
последующий force-push. Все шаги домашнего задания (Part I–III) выполнены,
оба pull-request успешно смержены, удалённые ветки удалены.
