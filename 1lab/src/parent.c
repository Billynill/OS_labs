#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    char fileName[256];
    char prompt[] = "Введите имя файла для результатов (например, results.txt): ";
    write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);

    ssize_t bytesRead = read(STDIN_FILENO, fileName, sizeof(fileName) - 1);
    if (bytesRead <= 0) {
        char errorMsg[] = "Ошибка: не удалось прочитать имя файла\n";
        write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
        return 1;
    }
    fileName[bytesRead] = '\0';

    size_t len = strlen(fileName);
    if (len > 0 && fileName[len - 1] == '\n') {
        fileName[len - 1] = '\0';
    }

    if (strlen(fileName) == 0) {
        char errorMsg[] = "Ошибка: имя файла пустое\n";
        write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
        return 1;
    }

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1) {
        char errorMsg[] = "pipe1: ";
        write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
        perror("");
        return 1;
    }

    if (pipe(pipe2) == -1) {
        char errorMsg[] = "pipe2: ";
        write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
        perror("");
        close(pipe1[0]);
        close(pipe1[1]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        char errorMsg[] = "fork: ";
        write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
        perror("");
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
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

    char infoMsg1[] = "Теперь вводите строки с числами (float через точку), например: 1.5 2 3\n";
    char infoMsg2[] = "Для завершения введите: exit\n";
    write(STDOUT_FILENO, infoMsg1, sizeof(infoMsg1) - 1);
    write(STDOUT_FILENO, infoMsg2, sizeof(infoMsg2) - 1);

    char line[1024];
    char childReply[512];

    while (1) {
        char promptSymbol[] = "> ";
        write(STDOUT_FILENO, promptSymbol, sizeof(promptSymbol) - 1);

        bytesRead = read(STDIN_FILENO, line, sizeof(line) - 1);
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                char eofMsg[] = "\nКонец ввода\n";
                write(STDOUT_FILENO, eofMsg, sizeof(eofMsg) - 1);
            } else {
                perror("read stdin");
            }
            break;
        }
        line[bytesRead] = '\0';

        size_t l = strlen(line);
        if (l > 0 && line[l - 1] == '\n') {
            line[l - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        char lineWithNewline[1024];
        snprintf(lineWithNewline, sizeof(lineWithNewline), "%s\n", line);
        ssize_t written = write(pipe1[1], lineWithNewline, strlen(lineWithNewline));
        if (written <= 0) {
            perror("write to child");
            break;
        }

        bytesRead = read(pipe2[0], childReply, sizeof(childReply) - 1);
        if (bytesRead > 0) {
            childReply[bytesRead] = '\0';
            char prefix[] = "[child] ";
            write(STDOUT_FILENO, prefix, sizeof(prefix) - 1);
            write(STDOUT_FILENO, childReply, bytesRead);
        } else {
            if (bytesRead == 0) {
                char closedMsg[] = "Дочерний процесс закрыл канал\n";
                write(STDOUT_FILENO, closedMsg, sizeof(closedMsg) - 1);
            } else {
                perror("read from child");
            }
            break;
        }
    }

    close(pipe1[1]);
    close(pipe2[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        char doneMsg[100];
        if (code == 0) {
            snprintf(doneMsg, sizeof(doneMsg), "Готово. Дочерний процесс завершился успешно.\n");
        } else {
            snprintf(doneMsg, sizeof(doneMsg), "Дочерний процесс завершился с кодом %d.\n", code);
        }
        write(STDOUT_FILENO, doneMsg, strlen(doneMsg));
    } else if (WIFSIGNALED(status)) {
        char signalMsg[100];
        snprintf(signalMsg, sizeof(signalMsg), "Дочерний процесс завершён сигналом %d.\n", WTERMSIG(status));
        write(STDOUT_FILENO, signalMsg, strlen(signalMsg));
    }

    return 0;
}
