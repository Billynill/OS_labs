#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    char fileName[256];
    printf("Введите имя файла для результатов (например, results.txt): ");
    fflush(stdout);

    if (!fgets(fileName, sizeof(fileName), stdin)) {
        fprintf(stderr, "Ошибка: не удалось прочитать имя файла\n");
        return 1;
    }

    size_t len = strlen(fileName);
    if (len > 0 && fileName[len - 1] == '\n') {
        fileName[len - 1] = '\0';
    }

    if (strlen(fileName) == 0) {
        fprintf(stderr, "Ошибка: имя файла пустое\n");
        return 1;
    }

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1) {
        perror("pipe1");
        return 1;
    }

    if (pipe(pipe2) == -1) {
        perror("pipe2");
        close(pipe1[0]);
        close(pipe1[1]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        return 1;
    }

    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2 stdin");
            _exit(1);
        }
        if (dup2(pipe2[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            _exit(1);
        }

        close(pipe1[0]);
        close(pipe2[1]);

        execl("./child", "child", fileName, (char*)NULL);
        perror("execl child");
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[1]);

    FILE *toChild = fdopen(pipe1[1], "w");
    if (!toChild) {
        perror("fdopen toChild");
        close(pipe1[1]);
        close(pipe2[0]);
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        return 1;
    }

    FILE *fromChild = fdopen(pipe2[0], "r");
    if (!fromChild) {
        perror("fdopen fromChild");
        fclose(toChild);
        close(pipe2[0]);
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        return 1;
    }

    printf("Теперь вводите строки с числами (float через точку), например: 1.5 2 3\n");
    printf("Для завершения введите: exit\n");

    char line[1024];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            if (feof(stdin)) {
                printf("\nКонец ввода\n");
            } else {
                perror("fgets stdin");
            }
            break;
        }

        size_t l = strlen(line);
        if (l > 0 && line[l - 1] == '\n') line[l - 1] = '\0';

        if (strcmp(line, "exit") == 0) {
            break;
        }

        if (fprintf(toChild, "%s\n", line) < 0) {
            perror("fprintf toChild");
            break;
        }
        fflush(toChild);

        char reply[512];
        if (fgets(reply, sizeof(reply), fromChild)) {
            size_t rl = strlen(reply);
            if (rl > 0 && reply[rl - 1] == '\n') reply[rl - 1] = '\0';
            printf("[child] %s\n", reply);
        } else {
            if (feof(fromChild)) {
                printf("Дочерний процесс закрыл канал\n");
            } else {
                perror("fgets fromChild");
            }
            break;
        }
    }

    fclose(toChild);
    fclose(fromChild);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 0) {
            printf("Готово. Дочерний процесс завершился успешно.\n");
        } else {
            printf("Дочерний процесс завершился с кодом %d.\n", code);
        }
    } else if (WIFSIGNALED(status)) {
        printf("Дочерний процесс завершён сигналом %d.\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("Дочерний процесс остановлен сигналом %d.\n", WSTOPSIG(status));
    }

    return 0;
}