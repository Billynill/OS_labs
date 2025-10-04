#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

static int SumFloatsInLine(const char *line, float *outSum, int *outCount) {
    if (!line || !outSum || !outCount) return -1;
    float sum = 0.0f;
    int count = 0;

    char buf[1024];
    if (strlen(line) >= sizeof(buf)) return -1;
    strcpy(buf, line);

    char *token = strtok(buf, " \t");
    while (token) {
        char *endptr = NULL;
        errno = 0;
        float val = strtof(token, &endptr);

        if (endptr == token || *endptr != '\0') {
        } else if (errno == ERANGE) {
        } else {
            sum += val;
            count++;
        }
        token = strtok(NULL, " \t");
    }
    *outSum = sum;
    *outCount = count;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        char usageMsg[100];
        snprintf(usageMsg, sizeof(usageMsg), "Использование: %s <output_file>\n", argv[0]);
        write(STDERR_FILENO, usageMsg, strlen(usageMsg));
        return 1;
    }

    const char *outPath = argv[1];
    int fout = open(outPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fout == -1) {
        char errorMsg[] = "fopen output: ";
        write(STDERR_FILENO, errorMsg, sizeof(errorMsg) - 1);
        perror("");
        return 1;
    }

    char line[1024];
    char resultMsg[512];
    char errorMsg[100];

    while (1) {
        ssize_t bytesRead = read(STDIN_FILENO, line, sizeof(line) - 1);
        if (bytesRead <= 0) {
            if (bytesRead < 0) {
                char readError[] = "fgets stdin: ";
                write(STDERR_FILENO, readError, sizeof(readError) - 1);
                perror("");
            }
            break;
        }
        line[bytesRead] = '\0';

        size_t l = strlen(line);
        if (l > 0 && line[l-1] == '\n') line[l-1] = '\0';

        float sum = 0.0f;
        int count = 0;

        if (SumFloatsInLine(line, &sum, &count) != 0) {
            char parseError[] = "error: parse line\n";
            write(STDERR_FILENO, parseError, sizeof(parseError) - 1);
            continue;
        }

        if (count == 0) {
            char noNumbers[] = "no numbers\n";
            write(STDOUT_FILENO, noNumbers, sizeof(noNumbers) - 1);
            continue;
        }

        char fileLine[1024];
        snprintf(fileLine, sizeof(fileLine), "line: \"%s\" sum: %.6f count: %d\n", line, sum, count);
        ssize_t written = write(fout, fileLine, strlen(fileLine));
        if (written <= 0) {
            char writeError[] = "error: write to file\n";
            write(STDERR_FILENO, writeError, sizeof(writeError) - 1);
        }

        snprintf(resultMsg, sizeof(resultMsg), "sum=%.6f count=%d\n", sum, count);
        write(STDOUT_FILENO, resultMsg, strlen(resultMsg));
    }

    close(fout);
    return 0;
}