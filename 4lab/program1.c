//
// Created by Рустам on 29.11.2025.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// Объявления функций из библиотеки
float pi(int k);
float e(int x);

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

int main() {
    char welcome[] = "Program 1 - Static linking\nCommands:\n0 - program info\n1 K - calculate pi with K terms\n2 X - calculate e with X terms\nexit - exit program\n\n";
    write(STDOUT_FILENO, welcome, strlen(welcome));

    char buffer[256];

    while (1) {
        char prompt[] = "Enter command: ";
        write(STDOUT_FILENO, prompt, strlen(prompt));

        int bytes_read = read_input(buffer, sizeof(buffer));
        if (bytes_read <= 0) break;

        // Обработка команды "0"
        if (strcmp(buffer, "0") == 0) {
            char msg[] = "Program 1 uses static linking with library.\nImplementation switching is not available.\n";
            write(STDOUT_FILENO, msg, strlen(msg));
        }
        // Обработка команды "exit"
        else if (strcmp(buffer, "exit") == 0) {
            break;
        }
        // Обработка команды "1" - вычисление π
        else if (buffer[0] == '1' && buffer[1] == ' ') {
            char *args = buffer + 2;
            int k = 0;

            // Парсинг аргумента
            char *token = strtok(args, " ");
            if (token != NULL) {
                k = parse_int(token);
            }

            if (k > 0) {
                float result = pi(k);

                char result_msg[100] = "pi = ";
                char num_str[50];
                float_to_string(result, num_str, sizeof(num_str));
                strcat(result_msg, num_str);
                strcat(result_msg, "\n");

                write(STDOUT_FILENO, result_msg, strlen(result_msg));
            } else {
                char error[] = "Error: invalid argument for command 1\nUsage: 1 K (K > 0)\n";
                write(STDOUT_FILENO, error, strlen(error));
            }
        }
        // Обработка команды "2" - вычисление e
        else if (buffer[0] == '2' && buffer[1] == ' ') {
            char *args = buffer + 2;
            int x = 0;

            // Парсинг аргумента
            char *token = strtok(args, " ");
            if (token != NULL) {
                x = parse_int(token);
            }

            if (x > 0) {
                float result = e(x);

                char result_msg[100] = "e = ";
                char num_str[50];
                float_to_string(result, num_str, sizeof(num_str));
                strcat(result_msg, num_str);
                strcat(result_msg, "\n");

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

    char exit_msg[] = "Program exit.\n";
    write(STDOUT_FILENO, exit_msg, strlen(exit_msg));
    return 0;
}