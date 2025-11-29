//
// Created by Рустам on 29.11.2025.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

// Типы для функций
typedef float (*pi_func)(int);
typedef float (*e_func)(int);

// Глобальные переменные для загруженных функций
static pi_func pi_ptr = NULL;
static e_func e_ptr = NULL;
static void *current_library = NULL;
static int current_lib_num = 1;

// Функция для преобразования числа в строку
void float_to_string(float value, char *buffer, int buffer_size) {
    if (buffer_size < 2) return;

    if (value < 0) {
        strcpy(buffer, "-");
        value = -value;
    } else {
        buffer[0] = '\0';
    }

    int integer_part = (int)value;
    float fractional_part = value - integer_part;

    // Конвертируем целую часть
    char int_str[20];
    int i = 0;

    if (integer_part == 0) {
        strcpy(int_str, "0");
    } else {
        char temp[20];
        int idx = 0;
        while (integer_part > 0) {
            temp[idx++] = '0' + (integer_part % 10);
            integer_part /= 10;
        }
        for (int j = idx - 1; j >= 0; j--) {
            int_str[i++] = temp[j];
        }
        int_str[i] = '\0';
    }

    strcat(buffer, int_str);
    strcat(buffer, ".");

    // Конвертируем дробную часть (6 знаков)
    fractional_part *= 1000000;
    int frac_int = (int)(fractional_part + 0.5);

    char frac_str[10];
    i = 0;
    for (int j = 100000; j >= 1; j /= 10) {
        frac_str[i++] = '0' + (frac_int / j);
        frac_int %= j;
    }
    frac_str[6] = '\0';

    strcat(buffer, frac_str);
}

void int_to_string(int value, char *buffer) {
    if (value == 0) {
        strcpy(buffer, "0");
        return;
    }

    int start_idx = 0;
    if (value < 0) {
        buffer[0] = '-';
        value = -value;
        start_idx = 1;
    }

    char temp[20];
    int idx = 0;

    while (value > 0) {
        temp[idx++] = '0' + (value % 10);
        value /= 10;
    }

    for (int i = 0; i < idx; i++) {
        buffer[start_idx + i] = temp[idx - 1 - i];
    }
    buffer[start_idx + idx] = '\0';
}

// Функция для чтения ввода
int read_input(char *buffer, int buffer_size) {
    int bytes_read = 0;
    char ch;

    while (bytes_read < buffer_size - 1) {
        if (read(STDIN_FILENO, &ch, 1) <= 0) {
            break;
        }
        if (ch == '\n') {
            break;
        }
        buffer[bytes_read++] = ch;
    }

    buffer[bytes_read] = '\0';
    return bytes_read;
}

// Функция для парсинга int
int parse_int(const char *str) {
    int result = 0;
    int sign = 1;

    if (*str == '-') {
        sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

// Функция для загрузки библиотеки
int load_library(int lib_num) {
    char lib_name[50];
    // Создаем имя библиотеки вручную
    strcpy(lib_name, "./libmethod");
    char num_str[3];
    int_to_string(lib_num, num_str);
    strcat(lib_name, num_str);
    strcat(lib_name, ".dylib"); // для macOS

    void *library = dlopen(lib_name, RTLD_LAZY);
    if (!library) {
        char error_msg[150] = "Error loading library: ";
        strcat(error_msg, dlerror());
        strcat(error_msg, "\n");
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 0;
    }

    // Загружаем функции
    pi_func new_pi = (pi_func)dlsym(library, "pi");
    e_func new_e = (e_func)dlsym(library, "e");

    if (!new_pi || !new_e) {
        char error_msg[] = "Error loading functions from library\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        dlclose(library);
        return 0;
    }

    // Закрываем предыдущую библиотеку
    if (current_library) {
        dlclose(current_library);
    }

    // Устанавливаем новые функции
    current_library = library;
    pi_ptr = new_pi;
    e_ptr = new_e;
    current_lib_num = lib_num;

    char success_msg[100] = "Library loaded: ";
    char num_msg[3];
    int_to_string(lib_num, num_msg);
    strcat(success_msg, num_msg);
    strcat(success_msg, "\n");
    write(STDOUT_FILENO, success_msg, strlen(success_msg));

    return 1;
}

int main() {
    char welcome[] = "Program 2 - Dynamic library loading\nCommands:\n0 - switch implementation\n1 K - calculate pi with K terms\n2 X - calculate e with X terms\nexit - exit program\n\n";
    write(STDOUT_FILENO, welcome, strlen(welcome));

    // Загружаем первую библиотеку по умолчанию
    if (!load_library(1)) {
        char error_msg[] = "Failed to load default library\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    char buffer[256];

    while (1) {
        char prompt[100] = "Enter command (library ";
        char num_str[3];
        int_to_string(current_lib_num, num_str);
        strcat(prompt, num_str);
        strcat(prompt, "): ");
        write(STDOUT_FILENO, prompt, strlen(prompt));

        int bytes_read = read_input(buffer, sizeof(buffer));
        if (bytes_read <= 0) break;

        // Обработка команды "0" - переключение библиотек
        if (strcmp(buffer, "0") == 0) {
            int new_lib_num = (current_lib_num == 1) ? 2 : 1;

            if (load_library(new_lib_num)) {
                char msg[100] = "Switched to library ";
                char num_msg[3];
                int_to_string(new_lib_num, num_msg);
                strcat(msg, num_msg);
                strcat(msg, "\n");
                write(STDOUT_FILENO, msg, strlen(msg));
            } else {
                char error[] = "Error switching library\n";
                write(STDOUT_FILENO, error, strlen(error));
            }
        }
        // Обработка команды "exit"
        else if (strcmp(buffer, "exit") == 0) {
            break;
        }
        // Обработка команды "1" - вычисление π
        else if (buffer[0] == '1' && buffer[1] == ' ') {
            if (!pi_ptr) {
                char error[] = "Error: pi function not loaded\n";
                write(STDOUT_FILENO, error, strlen(error));
                continue;
            }

            char *args = buffer + 2;
            int k = 0;

            char *token = strtok(args, " ");
            if (token != NULL) {
                k = parse_int(token);
            }

            if (k > 0) {
                float result = pi_ptr(k);

                char result_msg[150] = "pi = ";
                char num_str[50];
                float_to_string(result, num_str, sizeof(num_str));
                strcat(result_msg, num_str);
                strcat(result_msg, " (library ");
                char lib_str[3];
                int_to_string(current_lib_num, lib_str);
                strcat(result_msg, lib_str);
                strcat(result_msg, ")\n");

                write(STDOUT_FILENO, result_msg, strlen(result_msg));
            } else {
                char error[] = "Error: invalid argument for command 1\nUsage: 1 K (K > 0)\n";
                write(STDOUT_FILENO, error, strlen(error));
            }
        }
        // Обработка команды "2" - вычисление e
        else if (buffer[0] == '2' && buffer[1] == ' ') {
            if (!e_ptr) {
                char error[] = "Error: e function not loaded\n";
                write(STDOUT_FILENO, error, strlen(error));
                continue;
            }

            char *args = buffer + 2;
            int x = 0;

            char *token = strtok(args, " ");
            if (token != NULL) {
                x = parse_int(token);
            }

            if (x > 0) {
                float result = e_ptr(x);

                char result_msg[150] = "e = ";
                char num_str[50];
                float_to_string(result, num_str, sizeof(num_str));
                strcat(result_msg, num_str);
                strcat(result_msg, " (library ");
                char lib_str[3];
                int_to_string(current_lib_num, lib_str);
                strcat(result_msg, lib_str);
                strcat(result_msg, ")\n");

                write(STDOUT_FILENO, result_msg, strlen(result_msg));
            } else {
                char error[] = "Error: invalid argument for command 2\nUsage: 2 X (X > 0)\n";
                write(STDOUT_FILENO, error, strlen(error));
            }
        } else {
            char error[150];
            strcpy(error, "Unknown command: ");
            strcat(error, buffer);
            strcat(error, "\nAvailable commands: 0, 1 K, 2 X, exit\n");
            write(STDOUT_FILENO, error, strlen(error));
        }
    }

    // Закрываем библиотеку при выходе
    if (current_library) {
        dlclose(current_library);
    }

    char exit_msg[] = "Program exit.\n";
    write(STDOUT_FILENO, exit_msg, strlen(exit_msg));
    return 0;
}