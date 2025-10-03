#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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
        fprintf(stderr, "Использование: %s <output_file>\n", argv[0]);
        return 1;
    }

    const char *outPath = argv[1];
    FILE *fout = fopen(outPath, "a");
    if (!fout) {
        perror("fopen output");
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        if (ferror(stdin)) {
            perror("fgets stdin");
            break;
        }

        size_t l = strlen(line);
        if (l > 0 && line[l-1] == '\n') line[l-1] = '\0';

        float sum = 0.0f;
        int count = 0;

        if (SumFloatsInLine(line, &sum, &count) != 0) {
            fprintf(stderr, "error: parse line\n");
            continue;
        }

        if (count == 0) {
            printf("no numbers\n");
            fflush(stdout);
            continue;
        }

        if (fprintf(fout, "line: \"%s\" sum: %.6f count: %d\n", line, sum, count) < 0) {
            fprintf(stderr, "error: write to file\n");
        }
        fflush(fout);

        printf("sum=%.6f count=%d\n", sum, count);
        fflush(stdout);
    }

    fclose(fout);
    return 0;
}