#include "array_sum.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

int main() {
    std::vector<int32_t> v(1000000);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis(-100, 100);

    for (int32_t& num : v) {
        num = dis(gen);
    }

    auto start_basic = std::chrono::high_resolution_clock::now();
    int64_t sum_basic_func = sum_array_basic(v);
    auto end_basic = std::chrono::high_resolution_clock::now();

    auto duration_basic = std::chrono::duration_cast<std::chrono::nanoseconds>(end_basic - start_basic);

    auto start_neon = std::chrono::high_resolution_clock::now();
    int64_t sum_neon_func = sum_array_neon(v);
    auto end_neon = std::chrono::high_resolution_clock::now();

    auto duration_neon = std::chrono::duration_cast<std::chrono::nanoseconds>(end_neon - start_neon);

    std::cout << "Basic sum: " << sum_basic_func << '\n';
    std::cout << "NEON sum:  " << sum_neon_func << '\n';
    std::cout << "Basic time: " << duration_basic.count() << " ns\n";
    std::cout << "NEON time:  " << duration_neon.count() << " ns\n";
    std::cout << "NEON status: "
              << (neon_enabled_at_compile_time() ? "enabled" : "fallback (scalar)") << '\n';

    return 0;
}
