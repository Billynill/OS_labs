//
// Created by Рустам on 29.11.2025.
//

#include <math.h>

// Первая функция: вычисление π формулой Валлиса
float pi(int k) {
    float pi_value = 1.0;
    int n;

    for (n = 1; n <= k; n++) {
        float numerator = 4.0 * n * n;
        float denominator = 4.0 * n * n - 1.0;
        pi_value *= numerator / denominator;
    }

    return pi_value * 2;
}

// Функция для вычисления факториала
long long factorial(int n) {
    long long result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

// Вторая функция: вычисление e суммой ряда 1/n!
float e(int x) {
    float e_value = 0.0;
    int n;

    for (n = 0; n <= x; n++) {
        e_value += 1.0 / factorial(n);
    }

    return e_value;
}