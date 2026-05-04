// Программа печатает приветствие пользователю по введённому им имени.
#include <iostream>
#include <string>

int main() {
    std::string name;
    // Считываем имя пользователя из стандартного потока ввода.
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    // Выводим приветствие в стандартный поток вывода.
    std::cout << "Hello world from " << name << std::endl;
    return 0;
}
