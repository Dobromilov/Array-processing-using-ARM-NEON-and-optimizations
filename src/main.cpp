#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstdint>

int64_t sum_array_basic(const std::vector<int32_t>& v) {
    int64_t sum = 0;

    for (int32_t num : v) {
        if (num > 0) {
            sum += num;
        } else if (num < 0) {
            sum += -(int64_t)num;
        }
    }

    return sum;
}


int main() {
    std::vector<int> v(500);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dis(-100, 100);

    for (int &num : v) {
        num = dis(gen);
    }

    auto start = std::chrono::high_resolution_clock::now();
    int sum_basic_func = sum_array_basic(v);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Время: " << duration.count() << " нс" << std::endl;
    // std::cout << sum_basic_func;

    std::cout << "\n";
    return 0;
}
