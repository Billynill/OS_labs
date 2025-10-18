#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

long long wins_p1 = 0;
long long wins_p2 = 0;
long long draws = 0;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline int roll_die(unsigned int *seed) {
    return (rand_r(seed) % 6) + 1;
}

void run_experiment(int K, unsigned int *seed, long long *p1, long long *p2, long long *dr) {
    int s1 = 0, s2 = 0;
    for (int i = 0; i < K; i++) {
        s1 += roll_die(seed) + roll_die(seed);
        s2 += roll_die(seed) + roll_die(seed);
    }
    if (s1 > s2) (*p1)++;
    else if (s2 > s1) (*p2)++;
    else (*dr)++;
}

void run_sequential(int K, long long N, long long *p1, long long *p2, long long *dr) {
    unsigned int seed = (unsigned int)(time(NULL) ^ (unsigned int)getpid());
    *p1 = *p2 = *dr = 0;
    for (long long i = 0; i < N; i++) {
        run_experiment(K, &seed, p1, p2, dr);
    }
}

typedef struct {
    long long exp_count;
    int K;
    unsigned int seed;
} thread_arg_t;

void* worker(void* arg) {
    thread_arg_t* a = (thread_arg_t*)arg;
    long long p1 = 0, p2 = 0, dr = 0;
    unsigned int seed = a->seed;

    for (long long i = 0; i < a->exp_count; i++) {
        run_experiment(a->K, &seed, &p1, &p2, &dr);
    }

    pthread_mutex_lock(&result_mutex);
    wins_p1 += p1;
    wins_p2 += p2;
    draws += dr;
    pthread_mutex_unlock(&result_mutex);

    free(a);
    return NULL;
}

void run_parallel(int K, long long N, int T) {
    wins_p1 = wins_p2 = draws = 0;
    unsigned int base_seed = (unsigned int)(time(NULL) ^ (unsigned int)getpid());
    long long remaining = N;
    unsigned int current_seed = base_seed;

    while (remaining > 0) {
        long long threads_to_launch = (remaining > T) ? T : remaining;
        pthread_t* threads = calloc(threads_to_launch, sizeof(pthread_t));
        if (!threads) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }

        long long exp_per = remaining / threads_to_launch;
        long long extra = remaining % threads_to_launch;

        for (long long i = 0; i < threads_to_launch; i++) {
            thread_arg_t* args = malloc(sizeof(thread_arg_t));
            if (!args) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            args->K = K;
            args->exp_count = exp_per + (i < extra ? 1 : 0);
            args->seed = current_seed++;
            if (pthread_create(&threads[i], NULL, worker, args) != 0) {
                fprintf(stderr, "pthread_create failed\n");
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

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <K> <N>\n", argv[0]);
        fprintf(stderr, "  K - number of throws per player\n");
        fprintf(stderr, "  N - total number of experiments\n");
        return EXIT_FAILURE;
    }

    int K = atoi(argv[1]);
    long long N = atoll(argv[2]);
    if (K <= 0 || N <= 0) {
        fprintf(stderr, "Error: K and N must be positive integers.\n");
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long long p1, p2, dr;
    run_sequential(K, N, &p1, &p2, &dr);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double t_seq_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    long long T_seq_ms = (long long)(t_seq_sec * 1000 + 0.5);

    int thread_counts[] = {1, 2, 4, 6, 8, 16, 128, 1024};
    int num_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);

    printf("\n");
    printf("Число потоков | Время исполнения (мс) | Ускорение | Эффективность\n");
    printf("--------------|------------------------|-----------|----------------\n");

    double speedup = (double)T_seq_ms / T_seq_ms; // = 1.0
    double efficiency = speedup / 1.0;
    printf("%-13d | %-23lld | %-9.2f | %-14.4f\n", 1, T_seq_ms, speedup, efficiency);

    for (int i = 1; i < num_configs; i++) {
        int T = thread_counts[i];

        clock_gettime(CLOCK_MONOTONIC, &start);
        run_parallel(K, N, T);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double t_par_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        long long T_par_ms = (long long)(t_par_sec * 1000 + 0.5);

        speedup = (double)T_seq_ms / T_par_ms;
        efficiency = speedup / T;

        printf("%-13d | %-23lld | %-9.2f | %-14.4f\n", T, T_par_ms, speedup, efficiency);
    }

    printf("\n");
    return EXIT_SUCCESS;
}