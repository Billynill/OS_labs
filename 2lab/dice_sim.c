#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// --- Вспомогательные функции вывода ---
void write_str(int fd, const char* s) {
    write(fd, s, strlen(s));
}

void write_ll(int fd, long long n) {
    if (n < 0) {
        write(fd, "-", 1);
        n = -n;
    }
    if (n == 0) {
        write(fd, "0", 1);
        return;
    }
    char buf[32];
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        write(fd, &buf[j], 1);
    }
}

void write_double_2(int fd, double d) {
    if (d < 0) {
        write(fd, "-", 1);
        d = -d;
    }
    long long whole = (long long)d;
    write_ll(fd, whole);
    write(fd, ".", 1);
    long long frac = (long long)((d - whole) * 100 + 0.5);
    if (frac >= 100) {
        whole++;
        frac = 0;
        write_ll(fd, whole);
        write(fd, ".00", 3);
        return;
    }
    char buf[3];
    buf[1] = '0' + (frac % 10);
    buf[0] = '0' + (frac / 10);
    write(fd, buf, 2);
}

void write_double_4(int fd, double d) {
    if (d < 0) {
        write(fd, "-", 1);
        d = -d;
    }
    long long whole = (long long)d;
    write_ll(fd, whole);
    write(fd, ".", 1);
    long long frac = (long long)((d - whole) * 10000 + 0.5);
    if (frac >= 10000) {
        whole++;
        frac = 0;
        write_ll(fd, whole);
        write(fd, ".0000", 5);
        return;
    }
    char buf[5];
    for (int i = 3; i >= 0; i--) {
        buf[i] = '0' + (frac % 10);
        frac /= 10;
    }
    write(fd, buf, 4);
}

long long wins_p1 = 0;
long long wins_p2 = 0;
long long draws = 0;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline int roll_die(unsigned int *seed) {
    return (rand_r(seed) % 6) + 1;
}

void run_experiment(int K, int p1_total, int p2_total,
                   unsigned int *seed, long long *p1_wins, long long *p2_wins, long long *dr) {
    int p1_round_score = 0, p2_round_score = 0;

    for (int i = 0; i < K; i++) {
        p1_round_score += roll_die(seed) + roll_die(seed);
        p2_round_score += roll_die(seed) + roll_die(seed);
    }

    int p1_final = p1_total + p1_round_score;
    int p2_final = p2_total + p2_round_score;

    if (p1_final > p2_final) (*p1_wins)++;
    else if (p2_final > p1_final) (*p2_wins)++;
    else (*dr)++;
}

void run_sequential(int K, int p1_total, int p2_total,
                   long long N, long long *p1, long long *p2, long long *dr) {
    unsigned int seed = (unsigned int)(time(NULL) ^ (unsigned int)getpid());
    *p1 = *p2 = *dr = 0;
    for (long long i = 0; i < N; i++) {
        run_experiment(K, p1_total, p2_total, &seed, p1, p2, dr);
    }
}

typedef struct {
    long long exp_count;
    int K;
    int p1_total;
    int p2_total;
    unsigned int seed;
} thread_arg_t;

void* worker(void* arg) {
    thread_arg_t* a = (thread_arg_t*)arg;
    long long p1 = 0, p2 = 0, dr = 0;
    unsigned int seed = a->seed;

    for (long long i = 0; i < a->exp_count; i++) {
        run_experiment(a->K, a->p1_total, a->p2_total, &seed, &p1, &p2, &dr);
    }

    pthread_mutex_lock(&result_mutex);
    wins_p1 += p1;
    wins_p2 += p2;
    draws += dr;
    pthread_mutex_unlock(&result_mutex);

    free(a);
    return NULL;
}

void run_parallel(int K, int p1_total, int p2_total,
                 long long N, int max_threads) {
    wins_p1 = wins_p2 = draws = 0;
    unsigned int base_seed = (unsigned int)(time(NULL) ^ (unsigned int)getpid());
    long long remaining = N;
    unsigned int current_seed = base_seed;

    while (remaining > 0) {
        long long threads_to_launch = (remaining > max_threads) ? max_threads : remaining;
        pthread_t* threads = calloc(threads_to_launch, sizeof(pthread_t));
        if (!threads) {
            write_str(STDERR_FILENO, "calloc failed\n");
            exit(EXIT_FAILURE);
        }

        long long exp_per = remaining / threads_to_launch;
        long long extra = remaining % threads_to_launch;

        for (long long i = 0; i < threads_to_launch; i++) {
            thread_arg_t* args = malloc(sizeof(thread_arg_t));
            if (!args) {
                write_str(STDERR_FILENO, "malloc failed\n");
                exit(EXIT_FAILURE);
            }
            args->K = K;
            args->p1_total = p1_total;
            args->p2_total = p2_total;
            args->exp_count = exp_per + (i < extra ? 1 : 0);
            args->seed = current_seed++;
            if (pthread_create(&threads[i], NULL, worker, args) != 0) {
                write_str(STDERR_FILENO, "pthread_create failed\n");
                exit(EXIT_FAILURE);
            }
        }

        for (long long i = 0; i < threads_to_launch; i++) {
            pthread_join(threads[i], NULL);
        }

        free(threads);
        remaining -= threads_to_launch * exp_per + extra;
    }
}

void print_table_header() {
    write_str(STDOUT_FILENO, "Число потоков | Время (мс) | Ускорение | Эффективность\n");
    write_str(STDOUT_FILENO, "--------------|------------|-----------|----------------\n");
}

void print_table_row(int threads, long long time_ms, double speedup, double efficiency) {
    // Число потоков
    write_ll(STDOUT_FILENO, threads);
    if (threads < 10) write_str(STDOUT_FILENO, " ");
    write_str(STDOUT_FILENO, "           | ");

    // Время
    write_ll(STDOUT_FILENO, time_ms);
    int time_len = 0, tmp = time_ms;
    if (tmp == 0) time_len = 1;
    while (tmp > 0) { time_len++; tmp /= 10; }
    for (int i = 0; i < 10 - time_len; i++) write(STDOUT_FILENO, " ", 1);
    write_str(STDOUT_FILENO, " | ");

    // Ускорение
    write_double_2(STDOUT_FILENO, speedup);
    write_str(STDOUT_FILENO, "       | ");

    // Эффективность
    write_double_4(STDOUT_FILENO, efficiency);
    write_str(STDOUT_FILENO, "\n");
}

long long parse_ll(const char* str) {
    long long result = 0;
    int i = 0;

    while (str[i] != '\0') {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
            i++;
        } else {
            break;
        }
    }

    return result;
}

int parse_int(const char* str) {
    return (int)parse_ll(str);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        write_str(STDERR_FILENO, "Usage: ");
        write_str(STDERR_FILENO, argv[0]);
        write_str(STDERR_FILENO, " <K> <p1_score> <p2_score> <N> <max_threads>\n");
        write_str(STDERR_FILENO, "  K - бросков на игрока\n");
        write_str(STDERR_FILENO, "  p1_score - очки игрока 1\n");
        write_str(STDERR_FILENO, "  p2_score - очки игрока 2\n");
        write_str(STDERR_FILENO, "  N - экспериментов\n");
        write_str(STDERR_FILENO, "  max_threads - максимальное число потоков\n");
        return EXIT_FAILURE;
    }

    int K = parse_int(argv[1]);
    int p1_total = parse_int(argv[2]);
    int p2_total = parse_int(argv[3]);
    long long N = parse_ll(argv[4]);
    int max_threads = parse_int(argv[5]);

    if (K <= 0 || p1_total < 0 || p2_total < 0 || N <= 0 || max_threads <= 0) {
        write_str(STDERR_FILENO, "Error: Все параметры должны быть положительными\n");
        return EXIT_FAILURE;
    }

    write_str(STDOUT_FILENO, "Параметры: K=");
    write_ll(STDOUT_FILENO, K);
    write_str(STDOUT_FILENO, ", p1=");
    write_ll(STDOUT_FILENO, p1_total);
    write_str(STDOUT_FILENO, ", p2=");
    write_ll(STDOUT_FILENO, p2_total);
    write_str(STDOUT_FILENO, ", N=");
    write_ll(STDOUT_FILENO, N);
    write_str(STDOUT_FILENO, "\n\n");

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long long p1_seq, p2_seq, dr_seq;
    run_sequential(K, p1_total, p2_total, N, &p1_seq, &p2_seq, &dr_seq);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double t_seq_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    long long T_seq_ms = (long long)(t_seq_sec * 1000 + 0.5);

    write_str(STDOUT_FILENO, "Последовательная версия: ");
    write_ll(STDOUT_FILENO, T_seq_ms);
    write_str(STDOUT_FILENO, " мс\n\n");

    print_table_header();

    int thread_counts[] = {1, 2, 4, 6, 8, 16, 32};
    int num_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);

    for (int i = 0; i < num_configs; i++) {
        int T = thread_counts[i];
        if (T > max_threads) continue;

        clock_gettime(CLOCK_MONOTONIC, &start);
        run_parallel(K, p1_total, p2_total, N, T);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double t_par_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        long long T_par_ms = (long long)(t_par_sec * 1000 + 0.5);

        double speedup = (double)T_seq_ms / T_par_ms;
        double efficiency = speedup / T;

        print_table_row(T, T_par_ms, speedup, efficiency);
    }

    write_str(STDOUT_FILENO, "\n");
    return EXIT_SUCCESS;
}