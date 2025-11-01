#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SIZE 1024

typedef struct {
    char data[SIZE];
    char filename[SIZE];
    int shutdown;
    int has_data;
} shared_memory;

void write_str(int fd, const char* str) {
    write(fd, str, strlen(str));
}

void int_to_str(int num, char* buffer) {
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    char buf[20];
    int i = 0;
    int n = num;

    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }

    int j = 0;
    while (i > 0) {
        buffer[j++] = buf[--i];
    }
    buffer[j] = '\0';
}

float parse_float(const char* str) {
    float result = 0.0f;
    float fraction = 0.0f;
    float divisor = 1.0f;
    int has_dot = 0;
    int is_negative = 0;
    int has_digits = 0;

    if (*str == '-') {
        is_negative = 1;
        str++;
    }

    while (*str) {
        if (*str >= '0' && *str <= '9') {
            has_digits = 1;
            if (has_dot) {
                divisor *= 10.0f;
                fraction = fraction * 10.0f + (*str - '0');
            } else {
                result = result * 10.0f + (*str - '0');
            }
        } else if (*str == '.' && !has_dot) {
            has_dot = 1;
        } else {
            break;
        }
        str++;
    }

    if (!has_digits) return 0.0f;

    result += fraction / divisor;
    return is_negative ? -result : result;
}

void float_to_str(float num, char* buffer) {
    int integer = (int)num;
    float fractional = num - integer;
    if (fractional < 0) fractional = -fractional;
    int fraction_int = (int)(fractional * 100 + 0.5f);

    char* ptr = buffer;

    if (num < 0) {
        *ptr++ = '-';
        integer = -integer;
    }

    if (integer == 0) {
        *ptr++ = '0';
    } else {
        char buf[20];
        int i = 0;
        int n = integer;

        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        while (i > 0) {
            *ptr++ = buf[--i];
        }
    }

    *ptr++ = '.';
    *ptr++ = '0' + (fraction_int / 10);
    *ptr++ = '0' + (fraction_int % 10);
    *ptr = '\0';
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        char usageMsg[] = "Использование: client <shm_name> <sem_server_name> <sem_client_name>\n";
        write_str(STDERR_FILENO, usageMsg);
        return 1;
    }

    const char* shm_name = argv[1];
    const char* sem_server_name = argv[2];
    const char* sem_client_name = argv[3];

    int fd = shm_open(shm_name, O_RDWR, 0666);
    if (fd == -1) {
        char errorMsg[] = "shm_open client: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        return 1;
    }

    shared_memory *shm = mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        char errorMsg[] = "mmap client: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        return 1;
    }

    sem_t *sem_server = sem_open(sem_server_name, 0);
    if (sem_server == SEM_FAILED) {
        char errorMsg[] = "sem_open server client: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        munmap(shm, sizeof(shared_memory));
        return 1;
    }

    sem_t *sem_client = sem_open(sem_client_name, 0);
    if (sem_client == SEM_FAILED) {
        char errorMsg[] = "sem_open client client: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        sem_close(sem_server);
        munmap(shm, sizeof(shared_memory));
        return 1;
    }

    int file_fd = open(shm->filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd == -1) {
        char errorMsg[] = "open file: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        sem_close(sem_server);
        sem_close(sem_client);
        munmap(shm, sizeof(shared_memory));
        return 1;
    }

    while (1) {
        sem_wait(sem_client);

        if (shm->shutdown) break;

        if (shm->has_data) {
            char input[SIZE];
            strcpy(input, shm->data);

            float sum = 0.0f;
            int count = 0;
            char* token = strtok(input, " \t");

            while (token != NULL) {
                float num = parse_float(token);
                if ((token[0] >= '0' && token[0] <= '9') ||
                    (token[0] == '-' && token[1] >= '0' && token[1] <= '9') ||
                    (token[0] == '.' && token[1] >= '0' && token[1] <= '9')) {
                    sum += num;
                    count++;
                }
                token = strtok(NULL, " \t");
            }

            char result[SIZE];
            if (count > 0) {
                char sum_str[50], count_str[20], avg_str[50];
                float average = sum / count;

                float_to_str(sum, sum_str);
                float_to_str(average, avg_str);

                int_to_str(count, count_str);

                char* result_ptr = result;
                const char* parts[] = {"sum=", sum_str, " count=", count_str};

                for (int i = 0; i < 4; i++) {
                    const char* part = parts[i];
                    while (*part) {
                        *result_ptr++ = *part++;
                    }
                }
                *result_ptr = '\0';

                char file_line[SIZE];
                char* file_ptr = file_line;
                const char* file_parts[] = {"line: \"", shm->data, "\" sum: ", sum_str, " count: ", count_str, "\n"};

                for (int i = 0; i < 7; i++) {
                    const char* part = file_parts[i];
                    while (*part) {
                        *file_ptr++ = *part++;
                    }
                }
                *file_ptr = '\0';

                write(file_fd, file_line, strlen(file_line));

            } else {
                strcpy(result, "no numbers");
            }

            strcpy(shm->data, result);
            shm->has_data = 0;
        }

        sem_post(sem_server);
    }

    close(file_fd);

    sem_close(sem_server);
    sem_close(sem_client);
    munmap(shm, sizeof(shared_memory));

    return 0;
}