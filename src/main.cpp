#include "array_sum.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;

struct BenchmarkRow {
    std::size_t size;
    int64_t basic_time_ns;
    int64_t neon_time_ns;
    double speedup;
    std::size_t repetitions;
};

struct SummaryMetrics {
    double average_speedup;
    double best_speedup;
    std::size_t best_size;
};

struct CliOptions {
    bool print_terminal_output = true;
    std::string json_output_path;
    bool show_help = false;
};

std::vector<int32_t> make_array(std::size_t size, std::mt19937& gen) {
    std::uniform_int_distribution<int32_t> dis(-100, 100);
    std::vector<int32_t> v(size);

    for (int32_t& num : v) {
        num = dis(gen);
    }

    return v;
}

std::size_t repetitions_for(std::size_t size) {
    if (size == 0) {
        return 1000;
    }

    constexpr std::size_t target_elements = 250000;
    std::size_t repetitions = target_elements / size;

    if (repetitions < 3) {
        return 3;
    }

    return repetitions;
}

std::vector<std::size_t> build_sizes_from_breakpoints(
    const std::vector<std::size_t>& breakpoints,
    std::size_t points_per_segment
) {
    if (breakpoints.empty()) {
        return {};
    }
    if (breakpoints.size() == 1 || points_per_segment < 2) {
        return breakpoints;
    }

    std::vector<std::size_t> sizes;
    sizes.reserve((breakpoints.size() - 1) * (points_per_segment - 1) + 1);
    sizes.push_back(breakpoints.front());

    for (std::size_t i = 0; i + 1 < breakpoints.size(); ++i) {
        std::size_t left = breakpoints[i];
        std::size_t right = breakpoints[i + 1];
        if (right < left) {
            std::swap(left, right);
        }

        for (std::size_t point = 1; point < points_per_segment; ++point) {
            double t = static_cast<double>(point) / static_cast<double>(points_per_segment - 1);
            double value = static_cast<double>(left) + static_cast<double>(right - left) * t;
            std::size_t next = static_cast<std::size_t>(std::llround(value));

            if (next <= sizes.back()) {
                if (sizes.back() >= right) {
                    continue;
                }
                next = sizes.back() + 1;
            }

            sizes.push_back(next);
        }

        if (sizes.back() != right) {
            sizes.push_back(right);
        }
    }

    return sizes;
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

constexpr int kSizeWidth = 12;
constexpr int kTimeWidth = 15;
constexpr int kSpeedupWidth = 10;
constexpr int kRepeatWidth = 12;
constexpr int kVerdictWidth = 16;
constexpr int kVisualWidth = 15;

bool supports_color_output() {
    if (std::getenv("NO_COLOR") != nullptr) {
        return false;
    }

#if defined(_WIN32)
    if (_isatty(_fileno(stdout)) == 0) {
        return false;
    }
#else
    if (isatty(fileno(stdout)) == 0) {
        return false;
    }
#endif

    const char* term = std::getenv("TERM");
    if (term == nullptr) {
        return false;
    }

    return std::string(term) != "dumb";
}

const char* color_for_speedup(double speedup) {
    if (speedup > 1.05) {
        return "\033[32m";
    }
    if (speedup < 0.95) {
        return "\033[31m";
    }
    return "\033[33m";
}

std::string speed_bar(double speedup) {
    constexpr int kHalf = 6;
    std::string left(kHalf, '.');
    std::string right(kHalf, '.');

    double distance = std::abs(speedup - 1.0);
    int fill = static_cast<int>(std::round(std::min(1.0, distance / 0.5) * kHalf));

    if (speedup > 1.0) {
        for (int i = 0; i < fill; ++i) {
            right[i] = '>';
        }
    } else if (speedup < 1.0) {
        for (int i = 0; i < fill; ++i) {
            left[kHalf - 1 - i] = '<';
        }
    }

    return left + "|" + right;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [--json <path>] [--no-terminal] [--help]\n"
              << "  --json <path>    Write benchmark report to JSON file\n"
              << "  --no-terminal    Do not print benchmark table to stdout\n"
              << "  --help           Show this help\n";
}

bool parse_cli_options(int argc, char** argv, CliOptions& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            options.show_help = true;
            return true;
        }

        if (arg == "--no-terminal") {
            options.print_terminal_output = false;
            continue;
        }

        if (arg == "--json") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --json requires a file path\n";
                return false;
            }
            options.json_output_path = argv[++i];
            continue;
        }

        std::cerr << "Error: unknown argument: " << arg << '\n';
        print_usage(argv[0]);
        return false;
    }

    return true;
}

void print_separator(char fill = '-') {
    std::cout << '+'
              << std::string(kSizeWidth, fill) << '+'
              << std::string(kTimeWidth, fill) << '+'
              << std::string(kTimeWidth, fill) << '+'
              << std::string(kSpeedupWidth, fill) << '+'
              << std::string(kRepeatWidth, fill) << '+'
              << std::string(kVerdictWidth, fill) << '+'
              << std::string(kVisualWidth, fill) << "+\n";
}

std::string verdict_for(double speedup) {
    if (speedup > 1.05) {
        return "NEON faster";
    }
    if (speedup < 0.95) {
        return "Scalar faster";
    }
    return "About equal";
}

void print_header() {
    print_separator('=');
    std::cout << "| " << std::left << std::setw(kSizeWidth - 1) << "array size"
              << "| " << std::setw(kTimeWidth - 1) << "basic, ns"
              << "| " << std::setw(kTimeWidth - 1) << "neon, ns"
              << "| " << std::setw(kSpeedupWidth - 1) << "speedup"
              << "| " << std::setw(kRepeatWidth - 1) << "repeats"
              << "| " << std::setw(kVerdictWidth - 1) << "verdict"
              << "| " << std::setw(kVisualWidth - 1) << "visual"
              << "|\n";
    print_separator('=');
    std::cout << "| " << std::setw(kSizeWidth - 1) << ""
              << "| " << std::setw(kTimeWidth - 1) << ""
              << "| " << std::setw(kTimeWidth - 1) << ""
              << "| " << std::setw(kSpeedupWidth - 1) << ""
              << "| " << std::setw(kRepeatWidth - 1) << ""
              << "| " << std::setw(kVerdictWidth - 1) << ""
              << "| " << std::setw(kVisualWidth - 1) << "<slow|fast>"
              << "|\n";
    print_separator();
}

void print_row(const BenchmarkRow& row, bool with_color) {
    std::ostringstream out;
    out << "| " << std::right << std::setw(kSizeWidth - 1) << row.size
        << "| " << std::setw(kTimeWidth - 1) << row.basic_time_ns
        << "| " << std::setw(kTimeWidth - 1) << row.neon_time_ns
        << "| " << std::setw(kSpeedupWidth - 1) << std::fixed << std::setprecision(2) << row.speedup
        << "| " << std::setw(kRepeatWidth - 1) << row.repetitions
        << "| " << std::left << std::setw(kVerdictWidth - 1) << verdict_for(row.speedup)
        << "| " << std::setw(kVisualWidth - 1) << speed_bar(row.speedup)
        << '|';

    if (with_color) {
        std::cout << color_for_speedup(row.speedup) << out.str() << "\033[0m\n";
    } else {
        std::cout << out.str() << '\n';
    }
}

SummaryMetrics compute_summary(const std::vector<BenchmarkRow>& rows) {
    SummaryMetrics summary{};
    if (rows.empty()) {
        return summary;
    }

    auto best_it = std::max_element(
        rows.begin(),
        rows.end(),
        [](const BenchmarkRow& lhs, const BenchmarkRow& rhs) {
            return lhs.speedup < rhs.speedup;
        }
    );

    for (const BenchmarkRow& row : rows) {
        summary.average_speedup += row.speedup;
    }

    summary.average_speedup /= static_cast<double>(rows.size());
    summary.best_speedup = best_it->speedup;
    summary.best_size = best_it->size;

    return summary;
}

void print_summary(const SummaryMetrics& summary) {
    std::cout << "\nSummary\n";
    std::cout << "  average speedup (basic/neon): " << std::fixed << std::setprecision(2) << summary.average_speedup << "x\n";
    std::cout << "  best speedup: " << std::fixed << std::setprecision(2) << summary.best_speedup
              << "x at array size " << summary.best_size << "\n";
    std::cout << "  note: speedup > 1.00 means NEON is faster\n";
}

bool write_json_report(
    const std::string& path,
    const std::vector<BenchmarkRow>& rows,
    const SummaryMetrics& summary,
    bool neon_enabled
) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n";
    out << "  \"neon_enabled\": " << (neon_enabled ? "true" : "false") << ",\n";
    out << "  \"metric\": \"average_time_ns_per_call\",\n";
    out << "  \"summary\": {\n";
    out << "    \"average_speedup\": " << std::fixed << std::setprecision(6) << summary.average_speedup << ",\n";
    out << "    \"best_speedup\": " << std::fixed << std::setprecision(6) << summary.best_speedup << ",\n";
    out << "    \"best_size\": " << summary.best_size << "\n";
    out << "  },\n";
    out << "  \"rows\": [\n";

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const BenchmarkRow& row = rows[i];
        out << "    {\n";
        out << "      \"size\": " << row.size << ",\n";
        out << "      \"basic_time_ns\": " << row.basic_time_ns << ",\n";
        out << "      \"neon_time_ns\": " << row.neon_time_ns << ",\n";
        out << "      \"speedup\": " << std::fixed << std::setprecision(6) << row.speedup << ",\n";
        out << "      \"repetitions\": " << row.repetitions << "\n";
        out << "    }";
        if (i + 1 < rows.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    return true;
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::size_t> size_breakpoints = {
        0, 10, 100, 1000, 10000,
        100000, 1000000, 10000000
    };
    constexpr std::size_t kPointsPerSegment = 10;
    const std::vector<std::size_t> sizes = build_sizes_from_breakpoints(size_breakpoints, kPointsPerSegment);

    std::mt19937 gen(42);
    volatile int64_t sink = 0;
    bool with_color = supports_color_output();
    CliOptions options;

    if (!parse_cli_options(argc, argv, options)) {
        return 1;
    }

    if (options.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (options.print_terminal_output) {
        std::cout << "Array processing benchmark\n";
        std::cout << "NEON: " << (neon_enabled_at_compile_time() ? "enabled" : "fallback (scalar)") << '\n';
        std::cout << "Metric: average time per call (ns)\n\n";
    }

    std::vector<BenchmarkRow> rows;
    rows.reserve(sizes.size());
    if (options.print_terminal_output) {
        print_header();
    }

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
        BenchmarkRow row{size, basic_time, neon_time, ratio, repetitions};
        rows.push_back(row);
        if (options.print_terminal_output) {
            print_row(row, with_color);
        }
    }

    SummaryMetrics summary = compute_summary(rows);

    if (options.print_terminal_output) {
        print_separator();
        print_summary(summary);
    }

    if (!options.json_output_path.empty()) {
        if (!write_json_report(options.json_output_path, rows, summary, neon_enabled_at_compile_time())) {
            std::cerr << "Error: failed to write JSON report to " << options.json_output_path << '\n';
            return 1;
        }

        if (options.print_terminal_output) {
            std::cout << "JSON report written to: " << options.json_output_path << "\n";
        }
    }

    if (sink == 0) {
        std::cerr << "Benchmark guard failed\n";
        return 1;
    }

    return 0;
}
