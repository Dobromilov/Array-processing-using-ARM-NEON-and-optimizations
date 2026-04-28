#include "array_sum.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

std::vector<int32_t> make_array(std::size_t size, std::mt19937& gen) {
    std::uniform_int_distribution<int32_t> dis(-100, 100);
    std::vector<int32_t> v(size);

    for (int32_t& num : v) {
        num = dis(gen);
    }

    return v;
}

std::size_t repetitions_for(std::size_t size) {
    constexpr std::size_t target_elements = 20000000;
    std::size_t repetitions = target_elements / size;

    if (repetitions < 20) {
        return 20;
    }

    return repetitions;
}

int64_t average_time_ns(
    const std::vector<int32_t>& v,
    std::size_t repetitions,
    int64_t (*sum_func)(const std::vector<int32_t>&),
    volatile int64_t& sink
) {
    auto start = Clock::now();

    for (std::size_t i = 0; i < repetitions; ++i) {
        sink += sum_func(v);
    }

    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    return duration.count() / static_cast<int64_t>(repetitions);
}

void print_separator() {
    std::cout << "+------------+----------------+----------------+---------------+\n";
}

} // namespace

int main() {
    const std::array<std::size_t, 5> sizes = {10, 100, 1000, 10000, 1000000};
    std::mt19937 gen(42);
    volatile int64_t sink = 0;

    std::cout << "NEON status: "
              << (neon_enabled_at_compile_time() ? "enabled" : "fallback (scalar)") << "\n\n";
    std::cout << "Average time per call, ns\n";

    print_separator();
    std::cout << "| кол-во     | basic          | neon           | коэффициент   |\n";
    print_separator();

    for (std::size_t size : sizes) {
        std::vector<int32_t> v = make_array(size, gen);
        int64_t basic_sum = sum_array_basic(v);
        int64_t neon_sum = sum_array_neon(v);

        if (basic_sum != neon_sum) {
            std::cerr << "Error: sums do not match for array size " << size << '\n';
            return 1;
        }

        std::size_t repetitions = repetitions_for(size);
        int64_t basic_time = average_time_ns(v, repetitions, sum_array_basic, sink);
        int64_t neon_time = average_time_ns(v, repetitions, sum_array_neon, sink);
        double ratio = neon_time > 0 ? static_cast<double>(basic_time) / neon_time : 0.0;

        std::cout << "| " << std::right << std::setw(10) << size
                  << " | " << std::setw(14) << basic_time
                  << " | " << std::setw(14) << neon_time
                  << " | " << std::setw(13) << std::fixed << std::setprecision(2) << ratio
                  << " |\n";
    }

    print_separator();
    std::cout << "Coefficient = basic / neon. More than 1.00 means NEON is faster.\n";

    if (sink == 0) {
        std::cerr << "Benchmark guard failed\n";
        return 1;
    }

    return 0;
}
