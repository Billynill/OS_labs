//
// Created by Рустам on 29.11.2025.
//

#include <math.h>

// Первая функция: вычисление π рядом Лейбница
float pi(int k) {
    float pi_value = 0.0;
    int n;

    for (n = 0; n < k; n++) {
        float term = 1.0 / (2 * n + 1);
        if (n % 2 == 0) {
            pi_value += term;
        } else {
            pi_value -= term;
        }
    }

    return pi_value * 4;
}

// Вторая функция: вычисление e по формуле (1 + 1/x)^x
float e(int x) {
    if (x <= 0) return 0.0;
    return powf(1.0 + 1.0 / x, x);
}