#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>

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

int main() {
    char filename[256];
    char prompt[] = "Введите имя файла для результатов (например, results.txt): ";
    write_str(STDOUT_FILENO, prompt);

    ssize_t bytesRead = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytesRead <= 0) {
        char errorMsg[] = "Ошибка: не удалось прочитать имя файла\n";
        write_str(STDERR_FILENO, errorMsg);
        return 1;
    }
    filename[bytesRead] = '\0';

    size_t len = strlen(filename);
    if (len > 0 && filename[len - 1] == '\n') {
        filename[len - 1] = '\0';
    }

    if (strlen(filename) == 0) {
        char errorMsg[] = "Ошибка: имя файла пустое\n";
        write_str(STDERR_FILENO, errorMsg);
        return 1;
    }

    pid_t my_pid = getpid();
    char shm_name[256];
    char sem_server_name[256];
    char sem_client_name[256];

    char pid_str[20];
    int_to_str(my_pid, pid_str);

    strcpy(shm_name, "/lab_shm_");
    strcat(shm_name, pid_str);

    strcpy(sem_server_name, "/lab_sem_server_");
    strcat(sem_server_name, pid_str);

    strcpy(sem_client_name, "/lab_sem_client_");
    strcat(sem_client_name, pid_str);

    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        char errorMsg[] = "shm_open: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        return 1;
    }

    if (ftruncate(fd, sizeof(shared_memory)) == -1) {
        char errorMsg[] = "ftruncate: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        shm_unlink(shm_name);
        return 1;
    }

    shared_memory *shm = mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        char errorMsg[] = "mmap: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        shm_unlink(shm_name);
        return 1;
    }

    memset(shm, 0, sizeof(shared_memory));
    strcpy(shm->filename, filename);
    shm->shutdown = 0;
    shm->has_data = 0;

    sem_t *sem_server = sem_open(sem_server_name, O_CREAT, 0666, 1);
    if (sem_server == SEM_FAILED) {
        char errorMsg[] = "sem_open server: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        munmap(shm, sizeof(shared_memory));
        shm_unlink(shm_name);
        return 1;
    }

    sem_t *sem_client = sem_open(sem_client_name, O_CREAT, 0666, 0);
    if (sem_client == SEM_FAILED) {
        char errorMsg[] = "sem_open client: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        sem_close(sem_server);
        sem_unlink(sem_server_name);
        munmap(shm, sizeof(shared_memory));
        shm_unlink(shm_name);
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        execl("./client", "client", shm_name, sem_server_name, sem_client_name, NULL);
        char errorMsg[] = "execl: ошибка\n";
        write_str(STDERR_FILENO, errorMsg);
        _exit(1);
    }

    char infoMsg1[] = "Теперь вводите строки с числами (float через точку), например: 1.5 2 3\n";
    char infoMsg2[] = "Для завершения введите: exit\n";
    write_str(STDOUT_FILENO, infoMsg1);
    write_str(STDOUT_FILENO, infoMsg2);

    char line[1024];
    while (1) {
        char promptSymbol[] = "> ";
        write_str(STDOUT_FILENO, promptSymbol);

        ssize_t bytes = read(STDIN_FILENO, line, sizeof(line) - 1);
        if (bytes <= 0) {
            if (bytes == 0) {
                char eofMsg[] = "\nКонец ввода\n";
                write_str(STDOUT_FILENO, eofMsg);
            } else {
                char errorMsg[] = "read stdin: ошибка\n";
                write_str(STDERR_FILENO, errorMsg);
            }
            break;
        }
        line[bytes] = '\0';

        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        if (strlen(line) == 0) {
            continue;
        }

        sem_wait(sem_server);

        strcpy(shm->data, line);
        shm->has_data = 1;

        sem_post(sem_client);

        sem_wait(sem_server);

        char prefix[] = "[client] ";
        write_str(STDOUT_FILENO, prefix);
        write_str(STDOUT_FILENO, shm->data);
        write_str(STDOUT_FILENO, "\n");

        shm->has_data = 0;
        sem_post(sem_server);
    }

    sem_wait(sem_server);
    shm->shutdown = 1;
    sem_post(sem_client);
    sem_post(sem_server);

    wait(NULL);

    sem_close(sem_server);
    sem_close(sem_client);
    sem_unlink(sem_server_name);
    sem_unlink(sem_client_name);
    munmap(shm, sizeof(shared_memory));
    shm_unlink(shm_name);

    char doneMsg[] = "Готово. Данные сохранены в файл.\n";
    write_str(STDOUT_FILENO, doneMsg);

    return 0;
}