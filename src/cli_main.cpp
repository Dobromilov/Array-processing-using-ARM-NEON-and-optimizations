#include "array_sum.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

struct BenchmarkRow {
    std::string label;
    double basic_us;
    double neon_us;
};

std::vector<int32_t> generate_random_data(int size) {
    std::vector<int32_t> data(size);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int32_t> dis(-100, 100);

    for (int32_t& value : data) {
        value = dis(gen);
    }

    return data;
}

template <typename Func>
double measure_us(Func&& func, int64_t& result) {
    const auto start = std::chrono::high_resolution_clock::now();
    result = func();
    const auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double, std::micro>(end - start).count();
}

}  // namespace

int main() {
    const std::vector<int> sizes = {
        10, 50, 100, 500, 1000, 5000,
        10000, 50000, 100000, 250000, 500000,
        750000, 1000000, 2500000, 5000000, 10000000
    };

    const std::vector<std::string> labels = {
        "10", "50", "100", "500", "1k", "5k",
        "10k", "50k", "100k", "250k", "500k",
        "750k", "1M", "2.5M", "5M", "10M"
    };

    std::vector<BenchmarkRow> rows;
    rows.reserve(sizes.size());

    std::cout << "NEON status: "
              << (neon_enabled_at_compile_time() ? "enabled" : "fallback")
              << '\n';
    std::cout << "Running console ARM benchmark...\n\n";

    for (std::size_t i = 0; i < sizes.size(); ++i) {
        const std::vector<int32_t> data = generate_random_data(sizes[i]);

        int64_t basic_sum = 0;
        const double basic_us = measure_us([&data]() {
            return sum_array_basic(data);
        }, basic_sum);

        int64_t neon_sum = 0;
        const double neon_us = measure_us([&data]() {
            return sum_array_neon(data);
        }, neon_sum);

        if (basic_sum != neon_sum) {
            std::cerr << "Result mismatch at size " << sizes[i]
                      << ": basic=" << basic_sum
                      << ", neon=" << neon_sum << '\n';
            return 1;
        }

        rows.push_back({labels[i], basic_us, neon_us});
    }

    std::cout << std::left << std::setw(12) << "Size"
              << std::right << std::setw(14) << "Basic, us"
              << std::setw(14) << "NEON, us"
              << std::setw(12) << "Speedup" << '\n';

    std::cout << std::string(52, '-') << '\n';

    for (const BenchmarkRow& row : rows) {
        const double speedup = row.neon_us > 0.0 ? row.basic_us / row.neon_us : 0.0;
        std::cout << std::left << std::setw(12) << row.label
                  << std::right << std::setw(14) << std::fixed << std::setprecision(2) << row.basic_us
                  << std::setw(14) << row.neon_us
                  << std::setw(11) << std::setprecision(2) << speedup << 'x'
                  << '\n';
    }

    return 0;
}
