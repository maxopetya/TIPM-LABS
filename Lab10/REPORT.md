# Лабораторная работа №10

**Тема:** Изучение систем развёртывания виртуальной среды на примере Vagrant
**Студент:** Приходько Максим Максимович, ИУ8-24
**Дата:** 20.05.2026

Окружение: WSL2 Ubuntu 24.04, Vagrant 2.3.4. Провайдер виртуализации (VirtualBox) в окружении не установлен.

## Tutorial

### Установка Vagrant

Переменные из задания:

```sh
export GITHUB_USERNAME=maxopetya
export PACKAGE_MANAGER=apt
```

Команда `apt install vagrant` пакет не находит — в репозиториях Ubuntu 24.04 его нет. Официальный apt-репозиторий HashiCorp и его сервер плагинов из текущего региона недоступны (ответ `404`, причина `geo`). Поэтому Vagrant поставлен из пакета Debian:

```sh
cd ~/workspace
wget https://deb.debian.org/debian/pool/main/v/vagrant/vagrant_2.3.4+dfsg-1+deb12u1_all.deb -O vagrant.deb
sudo apt install -y ./vagrant.deb
```

Эта сборка рассчитана на ruby версии ниже 3.2, а в Ubuntu 24.04 установлен ruby 3.2.3 — `vagrant` из-за этого не запускается. Ограничение версии убрано из gemspec пакета:

```sh
sudo sed -i '/required_ruby_version/d' \
  /usr/share/rubygems-integration/all/specifications/vagrant-2.3.4.gemspec
```

После этого Vagrant работает.

### Версия и инициализация

Внутри WSL Vagrant по умолчанию останавливается на старте и требует явно разрешить доступ к окружению Windows. В сессии выставлена переменная:

```sh
export VAGRANT_WSL_ENABLE_WINDOWS_ACCESS=1
```

```sh
$ vagrant version
Installed Version: 2.3.4
```

```sh
$ vagrant init bento/ubuntu-19.10
A `Vagrantfile` has been placed in this directory. You are now
ready to `vagrant up` your first virtual environment!
```

После `init` создаётся `Vagrantfile` — шаблон с подробными комментариями ко всем основным параметрам. Команда `less Vagrantfile` открывает его в постраничном просмотре; в файле задана только базовая строка `config.vm.box`, остальное закомментировано.

```sh
$ vagrant init -f -m bento/ubuntu-19.10
```

Ключ `-f` перезаписывает существующий файл, `-m` создаёт минимальный вариант без комментариев:

```ruby
Vagrant.configure("2") do |config|
  config.vm.box = "bento/ubuntu-19.10"
end
```

Каталог для общей папки с гостевой машиной:

```sh
$ mkdir shared
```

### Сборка Vagrantfile

Итоговая конфигурация собирается тремя дозаписями. Первая объявляет shell-скрипт инициализации гостя (установка Docker, разворачивание образа `fastide/ubuntu`, создание пользователя `developer`). Вторая открывает блок `Vagrant.configure` и подключает плагин `vagrant-vbguest`. Третья задаёт box, сеть, общую папку, параметры провайдера VirtualBox и провижининг скриптом. Готовый `Vagrantfile`:

```ruby
$script = <<-SCRIPT
sudo apt install docker.io -y
sudo docker pull fastide/ubuntu:19.04
sudo docker create -ti --name fastide fastide/ubuntu:19.04 bash
sudo docker cp fastide:/home/developer /home/
sudo useradd developer
sudo usermod -aG sudo developer
echo "developer:developer" | sudo chpasswd
sudo chown -R developer /home/developer
SCRIPT

Vagrant.configure("2") do |config|

  config.vagrant.plugins = ["vagrant-vbguest"]

  config.vm.box = "bento/ubuntu-19.10"
  config.vm.network "public_network"
  config.vm.synced_folder('shared', '/vagrant', type: 'rsync')

  config.vm.provider "virtualbox" do |vb|
    vb.gui = true
    vb.memory = "2048"
  end

  config.vm.provision "shell", inline: $script, privileged: true

  config.ssh.extra_args = "-tt"

end
```

Здесь `config.vm.box` — базовый образ, `config.vm.network "public_network"` — мост в локальную сеть, `synced_folder` с типом `rsync` синхронизирует локальную папку `shared` с `/vagrant` в гостевой машине, блок `provider "virtualbox"` задаёт показ GUI и 2 ГБ ОЗУ, `provision "shell"` выполняет объявленный скрипт при первом запуске.

### Плагины

Строка `config.vagrant.plugins = ["vagrant-vbguest"]` требует установленного плагина. Сервер плагинов HashiCorp из текущего региона недоступен, поэтому плагины ставились с зеркала rubygems.org:

```sh
$ vagrant plugin install vagrant-vbguest --plugin-source https://rubygems.org
Installed the plugin 'vagrant-vbguest (0.32.0)'!

$ vagrant plugin install vagrant-vmware-esxi --plugin-source https://rubygems.org
Installed the plugin 'vagrant-vmware-esxi (2.5.5)'!

$ vagrant plugin list
vagrant-vbguest (0.32.0, global)
vagrant-vmware-esxi (2.5.5, global)
```

`vagrant-vbguest` следит за совпадением версий VirtualBox Guest Additions в госте и хосте, `vagrant-vmware-esxi` добавляет провайдер для развёртывания на сервере VMware ESXi.

### Проверка конфигурации и запуск

```sh
$ vagrant validate
$ vagrant status
$ vagrant up --provider=virtualbox
Vagrant could not detect VirtualBox! Make sure VirtualBox is properly installed.
Vagrant uses the `VBoxManage` binary that ships with VirtualBox, and requires
this to be available on the PATH.
```

`validate`, `status` и `up` сообщают `No usable default provider could be found`. Шаги `vagrant up`, `vagrant port`, `vagrant ssh`, `vagrant snapshot push/pop`, `vagrant halt` требуют запущенной виртуальной машины и не выполнялись: провайдер VirtualBox в окружении не установлен.

ESXi-провайдер из задания (`config.vm.provider :vmware_esxi`, `vagrant up --provider=vmware_esxi`) требует доступа к отдельному серверу VMware ESXi с указанием его адреса и учётных данных — такого сервера в работе нет. Установка плагина `vagrant-vmware-esxi` и проверка его в `vagrant plugin list` выполнены.

## Ссылки

- VirtualBox: https://www.virtualbox.org/
- Vagrant providers: https://github.com/hashicorp/vagrant/wiki/Available-Vagrant-Plugins#providers
- Vagrant vbguest plugin: https://github.com/dotless-de/vagrant-vbguest
- Vagrant disksize plugin: https://github.com/sprotheroe/vagrant-disksize
- Vagrant vmware esxi plugin: https://github.com/josenk/vagrant-vmware-esxi

## Вывод

Vagrant — это надстройка над провайдерами виртуализации (VirtualBox, VMware, Hyper-V), которая описывает виртуальную среду разработки одним текстовым файлом `Vagrantfile`. В файле в декларативном виде задаётся базовый образ, ресурсы машины, сеть, общие папки и скрипты первоначальной настройки; одной команды `vagrant up` достаточно, чтобы из этого описания развернуть полностью готовое окружение, а `vagrant ssh`, `snapshot`, `halt` — управлять его жизненным циклом. За счёт того, что вся конфигурация лежит в репозитории рядом с кодом, у всех участников проекта окружение получается одинаковым, а смена провайдера (локальный VirtualBox или удалённый ESXi) сводится к подключению соответствующего плагина без переписывания самого описания. В этой работе пройдены установка Vagrant, инициализация проекта, сборка полного `Vagrantfile` и подключение плагинов; запуск самой виртуальной машины не выполнялся из-за отсутствия установленного провайдера.
