// Программа выводит приветствие пользователю по введённому имени.
#include <iostream>
#include <string>

int main() {
    std::string name;
    // Запрашиваем имя у пользователя.
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    // Печатаем приветствие в стандартный поток вывода.
    std::cout << "Hello world from " << name << std::endl;
    return 0;
}
