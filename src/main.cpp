#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>

void sum_array_basic(const std::vector<int>& v, int& sum_basic){
    sum_basic = 0;
    for (int num : v) {
        sum_basic += num;
    }
}

int main() {
    std::vector<int> v(50);
    int sum_basic;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    for (int &num : v) {
        num = dis(gen);
    }

    auto start = std::chrono::high_resolution_clock::now();
    sum_array_basic(v, sum_basic);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Время: " << duration.count() << " нс" << std::endl;
    // std::cout << sum_basic;

    std::cout << "\n";
    return 0;
}
