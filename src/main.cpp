#include "array_sum.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>
#include <iomanip>

int main() {
    std::vector<int> sizes = {10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis(-100, 100);

    std::cout << "\n==========================================================================================================================\n";
    std::cout << "| Размер массива |    Базовая сумма   |    NEON сумма      | Базовая время (мкс)  | NEON время (мкс)   | Ускорение (раз) |\n";
    std::cout << "==========================================================================================================================\n";

    for (int size : sizes) {
        std::vector<int32_t> v(size);

        for (int32_t& num : v) {
            num = dis(gen);
        }

        auto start_basic = std::chrono::high_resolution_clock::now();
        int64_t sum_basic_func = sum_array_basic(v);
        auto end_basic = std::chrono::high_resolution_clock::now();
        auto duration_basic = std::chrono::duration_cast<std::chrono::nanoseconds>(end_basic - start_basic);
        double time_basic_us = duration_basic.count() / 1000.0; // микросекунды

        auto start_neon = std::chrono::high_resolution_clock::now();
        int64_t sum_neon_func = sum_array_neon(v);
        auto end_neon = std::chrono::high_resolution_clock::now();
        auto duration_neon = std::chrono::duration_cast<std::chrono::nanoseconds>(end_neon - start_neon);
        double time_neon_us = duration_neon.count() / 1000.0; // микросекунды

        if (sum_basic_func != sum_neon_func) {
            std::cerr << "Ошибка: суммы не совпадают для размера " << size << "!\n";
            return 1;
        }

        double speedup = time_basic_us / time_neon_us;

        std::cout << "| " << std::setw(14) << size
                  << " | " << std::setw(18) << sum_basic_func
                  << " | " << std::setw(18) << sum_neon_func
                  << " | " << std::setw(20) << std::fixed << std::setprecision(2) << time_basic_us
                  << " | " << std::setw(18) << std::fixed << std::setprecision(2) << time_neon_us
                  << " | " << std::setw(14) << std::fixed << std::setprecision(2) << speedup << "x |\n";

        std::cout << "--------------------------------------------------------------------------------------------------------------------------\n";
    }

    return 0;
}