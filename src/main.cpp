#include "array_sum.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#if defined(ARRAY_USE_GLES)
#define GLFW_INCLUDE_ES2
#endif
#include <GLFW/glfw3.h>

class PerformanceBenchmark {
private:
    std::vector<int32_t> data;
    std::vector<float> basic_times;
    std::vector<float> neon_times;
    std::vector<int> array_sizes;
    bool is_benchmarking = false;
    float benchmark_progress = 0.0f;

    // Основные размеры для отображения на оси X (только степени 10)
    const std::vector<int> display_sizes = {
        100, 1000, 10000, 100000, 1000000
    };

    const std::vector<std::string> display_labels = {
        "100", "1k", "10k", "100k", "1M"
    };

    // Функция для генерации промежуточных точек между двумя размерами
    std::vector<int> generate_intermediate_sizes(int start, int end) {
        std::vector<int> sizes;
        if (start == end) {
            sizes.push_back(start);
            return sizes;
        }

        // Используем логарифмическую шкалу для более равномерного распределения
        double log_start = std::log10(start);
        double log_end = std::log10(end);

        // 100 промежуточных точек (включая края)
        for (int i = 0; i <= 100; i++) {
            double t = i / 100.0;
            double log_val = log_start + t * (log_end - log_start);
            int size = static_cast<int>(std::round(std::pow(10, log_val)));
            // Гарантируем уникальность и избегаем дубликатов
            if (sizes.empty() || sizes.back() != size) {
                sizes.push_back(size);
            }
        }
        return sizes;
    }

    // Функция для форматирования времени в секундах
    std::string format_time_seconds(float time_us) {
        float time_sec = time_us / 1000000.0f;
        std::ostringstream oss;
        if (time_sec < 0.000001f) {
            oss << std::fixed << std::setprecision(3) << (time_sec * 1000000000.0f) << " ns";
        } else if (time_sec < 0.001f) {
            oss << std::fixed << std::setprecision(3) << (time_sec * 1000000.0f) << " µs";
        } else if (time_sec < 1.0f) {
            oss << std::fixed << std::setprecision(3) << (time_sec * 1000.0f) << " ms";
        } else {
            oss << std::fixed << std::setprecision(6) << time_sec << " s";
        }
        return oss.str();
    }

public:
    PerformanceBenchmark() {
        run_full_benchmark();
    }

    void generate_random_data(int size) {
        data.resize(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int32_t> dis(-100, 100);

        for (int32_t& num : data) {
            num = dis(gen);
        }
    }

    void run_single_benchmark(int size) {
        generate_random_data(size);

        auto start_basic = std::chrono::high_resolution_clock::now();
        int64_t sum_basic = sum_array_basic(data);
        auto end_basic = std::chrono::high_resolution_clock::now();
        float basic_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_basic - start_basic).count() / 1000.0f;

        auto start_neon = std::chrono::high_resolution_clock::now();
        int64_t sum_neon = sum_array_neon(data);
        auto end_neon = std::chrono::high_resolution_clock::now();
        float neon_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_neon - start_neon).count() / 1000.0f;

        if (sum_basic != sum_neon) {
            std::cerr << "Warning: Results don't match! Basic: " << sum_basic
                      << ", NEON: " << sum_neon << std::endl;
        }

        basic_times.push_back(basic_time);
        neon_times.push_back(neon_time);
        array_sizes.push_back(size);
    }

    void run_full_benchmark() {
        is_benchmarking = true;
        basic_times.clear();
        neon_times.clear();
        array_sizes.clear();

        // Генерируем все размеры для бенчмаркинга (с промежуточными точками)
        std::vector<int> all_sizes;
        for (std::size_t i = 0; i < display_sizes.size() - 1; i++) {
            std::vector<int> intermediate = generate_intermediate_sizes(display_sizes[i], display_sizes[i + 1]);
            all_sizes.insert(all_sizes.end(), intermediate.begin(), intermediate.end());
            // Убираем дубликат последнего элемента (он будет первым в следующем интервале)
            if (!all_sizes.empty() && !intermediate.empty()) {
                all_sizes.pop_back();
            }
        }
        // Добавляем последний размер
        all_sizes.push_back(display_sizes.back());

        // Запускаем бенчмарки для всех размеров
        for (std::size_t i = 0; i < all_sizes.size(); i++) {
            run_single_benchmark(all_sizes[i]);
            benchmark_progress = static_cast<float>(i + 1) / all_sizes.size();
        }

        is_benchmarking = false;
    }

    void draw_table() {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        if (ImGui::BeginTable("PerformanceTable", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
            ImVec2(0, 400))) {  // Добавили вертикальную прокрутку

            ImGui::TableSetupColumn("Array Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Basic (s)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("NEON (s)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Speedup", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            // Отображаем ВСЕ точки в таблице
            for (std::size_t i = 0; i < array_sizes.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                // Форматируем размер с разделителями тысяч
                std::string size_str;
                int size = array_sizes[i];
                if (size >= 1000000) {
                    size_str = std::to_string(size / 1000000) + "M";
                } else if (size >= 1000) {
                    size_str = std::to_string(size / 1000) + "k";
                } else {
                    size_str = std::to_string(size);
                }
                ImGui::Text("%s", size_str.c_str());

                ImGui::TableSetColumnIndex(1);
                std::string time_str = format_time_seconds(basic_times[i]);
                ImGui::Text("%s", time_str.c_str());

                ImGui::TableSetColumnIndex(2);
                time_str = format_time_seconds(neon_times[i]);
                ImGui::Text("%s", time_str.c_str());

                ImGui::TableSetColumnIndex(3);
                float speedup = basic_times[i] / neon_times[i];
                ImVec4 color = speedup > 1.5f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
                               speedup > 1.0f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                               ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                ImGui::TextColored(color, "%.2fx", speedup);
            }

            ImGui::EndTable();
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    }

    void draw_combined_graph() {
        if (array_sizes.empty()) return;

        float max_time = 0;
        for (float t : basic_times) max_time = std::max(max_time, t);
        for (float t : neon_times) max_time = std::max(max_time, t);

        // Переводим в секунды для отображения
        float max_time_sec = max_time / 1000000.0f;
        float min_time_sec = 0.0f;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        ImGui::Text("Performance: Basic vs NEON (Log Scale for X, Linear for Y)");
        ImGui::Spacing();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 graph_pos = ImGui::GetCursorScreenPos();
        ImVec2 graph_size = ImVec2(ImGui::GetContentRegionAvail().x - 20, 400);

        draw_list->AddRectFilled(graph_pos, ImVec2(graph_pos.x + graph_size.x, graph_pos.y + graph_size.y),
                                 IM_COL32(20, 30, 60, 255));
        draw_list->AddRect(graph_pos, ImVec2(graph_pos.x + graph_size.x, graph_pos.y + graph_size.y),
                           IM_COL32(80, 100, 150, 255), 2.0f);

        // Линейная шкала для Y (время в секундах) - СДВИНУТА ВПРАВО на 30 пикселей
        float y_axis_offset = 30.0f;  // Сдвигаем ось Y вправо на 30px
        for (int i = 0; i <= 6; i++) {
            float y = graph_pos.y + (graph_size.y * i / 6.0f);
            draw_list->AddLine(ImVec2(graph_pos.x + y_axis_offset, y),
                              ImVec2(graph_pos.x + graph_size.x, y),
                               IM_COL32(50, 70, 110, 150), 1.0f);

            // Линейные значения в секундах
            float value = max_time_sec - (i / 6.0f) * max_time_sec;

            char y_label[64];
            if (value >= 0.00000001f) {
                snprintf(y_label, sizeof(y_label), "%.5f s", value);
            }
            draw_list->AddText(ImVec2(graph_pos.x - 55 + y_axis_offset, y - 6),
                              IM_COL32(200, 200, 220, 255), y_label);
        }

        // Логарифмическая шкала для X (размер массива)
        float min_size_log = std::log10(array_sizes.front());
        float max_size_log = std::log10(array_sizes.back());

        // Рисуем вертикальные линии для логарифмической шкалы X
        for (int i = 0; i <= 8; i++) {
            float log_val = min_size_log + (i / 8.0f) * (max_size_log - min_size_log);
            float size_val = std::pow(10, log_val);
            float x = graph_pos.x + y_axis_offset + ((log_val - min_size_log) / (max_size_log - min_size_log)) * (graph_size.x - y_axis_offset);

            if (x >= graph_pos.x && x <= graph_pos.x + graph_size.x) {
                draw_list->AddLine(ImVec2(x, graph_pos.y), ImVec2(x, graph_pos.y + graph_size.y),
                                  IM_COL32(50, 70, 110, 100), 1.0f);
            }
        }

        if (basic_times.size() > 1) {
            // Рисуем линию для Basic (линейная шкала Y)
            std::vector<ImVec2> basic_points;
            for (std::size_t i = 0; i < basic_times.size(); i++) {
                float time_sec = basic_times[i] / 1000000.0f;
                float size_log = std::log10(array_sizes[i]);
                float x = graph_pos.x + y_axis_offset + ((size_log - min_size_log) / (max_size_log - min_size_log)) * (graph_size.x - y_axis_offset);

                // Линейное преобразование Y
                float y = graph_pos.y + graph_size.y - (time_sec / max_time_sec) * graph_size.y;
                y = std::max(graph_pos.y, std::min(graph_pos.y + graph_size.y, y));
                basic_points.push_back(ImVec2(x, y));
            }

            for (std::size_t i = 0; i < basic_points.size() - 1; i++) {
                draw_list->AddLine(basic_points[i], basic_points[i + 1], IM_COL32(100, 150, 255, 255), 2.0f);
            }

            // Рисуем линию для NEON (линейная шкала Y)
            std::vector<ImVec2> neon_points;
            for (std::size_t i = 0; i < neon_times.size(); i++) {
                float time_sec = neon_times[i] / 1000000.0f;
                float size_log = std::log10(array_sizes[i]);
                float x = graph_pos.x + y_axis_offset + ((size_log - min_size_log) / (max_size_log - min_size_log)) * (graph_size.x - y_axis_offset);

                // Линейное преобразование Y
                float y = graph_pos.y + graph_size.y - (time_sec / max_time_sec) * graph_size.y;
                y = std::max(graph_pos.y, std::min(graph_pos.y + graph_size.y, y));
                neon_points.push_back(ImVec2(x, y));
            }

            for (std::size_t i = 0; i < neon_points.size() - 1; i++) {
                draw_list->AddLine(neon_points[i], neon_points[i + 1], IM_COL32(100, 255, 100, 255), 2.0f);
            }

            // Отмечаем основные точки на графике
            int display_idx = 0;
            for (std::size_t i = 0; i < array_sizes.size(); i++) {
                if (display_idx < display_sizes.size() && array_sizes[i] == display_sizes[display_idx]) {
                    float size_log = std::log10(array_sizes[i]);
                    float x = graph_pos.x + y_axis_offset + ((size_log - min_size_log) / (max_size_log - min_size_log)) * (graph_size.x - y_axis_offset);

                    // Basic точка
                    float time_sec_basic = basic_times[i] / 1000000.0f;
                    float y_basic = graph_pos.y + graph_size.y - (time_sec_basic / max_time_sec) * graph_size.y;
                    y_basic = std::max(graph_pos.y, std::min(graph_pos.y + graph_size.y, y_basic));

                    // NEON точка
                    float time_sec_neon = neon_times[i] / 1000000.0f;
                    float y_neon = graph_pos.y + graph_size.y - (time_sec_neon / max_time_sec) * graph_size.y;
                    y_neon = std::max(graph_pos.y, std::min(graph_pos.y + graph_size.y, y_neon));

                    draw_list->AddCircleFilled(ImVec2(x, y_basic), 5.0f, IM_COL32(100, 150, 255, 255));
                    draw_list->AddCircle(ImVec2(x, y_basic), 5.0f, IM_COL32(255, 255, 255, 200), 2.0f);
                    draw_list->AddCircleFilled(ImVec2(x, y_neon), 5.0f, IM_COL32(100, 255, 100, 255));
                    draw_list->AddCircle(ImVec2(x, y_neon), 5.0f, IM_COL32(255, 255, 255, 200), 2.0f);

                    display_idx++;
                }
            }
        }

        ImGui::Dummy(graph_size);

        // Отображаем метки на оси X (логарифмическая шкала)
        ImVec2 text_pos = ImGui::GetCursorScreenPos();

        // Отображаем метки для основных размеров
        int display_idx = 0;
        for (std::size_t i = 0; i < array_sizes.size(); i++) {
            if (display_idx < display_sizes.size() && array_sizes[i] == display_sizes[display_idx]) {
                float size_log = std::log10(array_sizes[i]);
                float x = graph_pos.x + y_axis_offset + ((size_log - min_size_log) / (max_size_log - min_size_log)) * (graph_size.x - y_axis_offset);
                x = x - 15;
                if (x < graph_pos.x + y_axis_offset) x = graph_pos.x + y_axis_offset;
                if (x > graph_pos.x + graph_size.x - 30) x = graph_pos.x + graph_size.x - 30;

                ImGui::GetWindowDrawList()->AddText(ImVec2(x, text_pos.y + 5),
                                                   IM_COL32(200, 200, 220, 255),
                                                   display_labels[display_idx].c_str());
                display_idx++;
            }
        }

        // Добавляем подпись к оси X
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::SetCursorPosX(graph_pos.x + graph_size.x / 2 - 50);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Array Size (log scale)");

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "NEON Optimized");
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "Basic Sum");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    }

    void draw_speedup_chart() {
        if (array_sizes.empty()) return;

        std::vector<float> speedups;
        std::vector<int> display_indices;

        // Собираем speedup только для основных размеров
        int display_idx = 0;
        for (std::size_t i = 0; i < basic_times.size(); i++) {
            if (display_idx < display_sizes.size() && array_sizes[i] == display_sizes[display_idx]) {
                speedups.push_back(basic_times[i] / neon_times[i]);
                display_indices.push_back(i);
                display_idx++;
            }
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        ImGui::Text("Speedup Factor (Log Scale)");
        ImGui::Spacing();

        float max_speedup = std::max(3.0f, *std::max_element(speedups.begin(), speedups.end()) + 0.5f);
        float min_speedup = 0.5f; // Минимальное значение для логарифмической шкалы

        float log_min_speedup = std::log10(min_speedup);
        float log_max_speedup = std::log10(max_speedup);

        float available_width = ImGui::GetContentRegionAvail().x - 20;
        float bar_width = (available_width - 40) / speedups.size();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 start_pos = ImGui::GetCursorScreenPos();

        // Рисуем сетку для логарифмической шкалы
        for (int i = 0; i <= 5; i++) {
            float y_pos = start_pos.y + 200 - (i / 5.0f) * 200;
            draw_list->AddLine(ImVec2(start_pos.x, y_pos),
                              ImVec2(start_pos.x + speedups.size() * bar_width + 5, y_pos),
                              IM_COL32(50, 70, 110, 150), 1.0f);

            float log_val = log_min_speedup + (i / 5.0f) * (log_max_speedup - log_min_speedup);
            float speed_val = std::pow(10, log_val);

            char label[32];
            snprintf(label, sizeof(label), "%.2fx", speed_val);
            draw_list->AddText(ImVec2(start_pos.x - 50, y_pos - 6),
                              IM_COL32(180, 180, 220, 255), label);
        }

        for (std::size_t i = 0; i < speedups.size(); i++) {
            float log_speedup = std::log10(std::max(min_speedup, speedups[i]));
            float bar_height = ((log_speedup - log_min_speedup) / (log_max_speedup - log_min_speedup)) * 200;

            ImVec2 bar_start = ImVec2(start_pos.x + i * bar_width + 5, start_pos.y + 200 - bar_height);
            ImVec2 bar_end = ImVec2(start_pos.x + (i + 1) * bar_width, start_pos.y + 200);

            ImU32 color = speedups[i] > 1.5f ? IM_COL32(0, 200, 0, 220) :
                          speedups[i] > 1.0f ? IM_COL32(200, 200, 0, 220) :
                          IM_COL32(200, 0, 0, 220);

            draw_list->AddRectFilled(bar_start, bar_end, color, 3.0f);
            draw_list->AddRect(bar_start, bar_end, IM_COL32(255, 255, 255, 100), 3.0f);

            char text[32];
            snprintf(text, sizeof(text), "%.2fx", speedups[i]);
            ImVec2 text_size = ImGui::CalcTextSize(text);
            draw_list->AddText(ImVec2(bar_start.x + (bar_width - text_size.x) / 2, bar_start.y - 20),
                              IM_COL32(255, 255, 255, 255), text);

            if (i < display_labels.size()) {
                ImVec2 label_size = ImGui::CalcTextSize(display_labels[i].c_str());
                draw_list->AddText(ImVec2(bar_start.x + (bar_width - label_size.x) / 2, start_pos.y + 205),
                                  IM_COL32(180, 180, 220, 255), display_labels[i].c_str());
            }
        }

        // Линия baseline (speedup = 1.0)
        float log_baseline = std::log10(1.0f);
        float baseline_y = start_pos.y + 200 - ((log_baseline - log_min_speedup) / (log_max_speedup - log_min_speedup)) * 200;
        draw_list->AddLine(ImVec2(start_pos.x, baseline_y),
                          ImVec2(start_pos.x + speedups.size() * bar_width + 5, baseline_y),
                          IM_COL32(255, 255, 255, 255), 2.0f);

        ImGui::Dummy(ImVec2(available_width, 240));

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    }

    void draw_controls() {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "NEON Status: %s",
                   neon_enabled_at_compile_time() ? "ENABLED" : "FALLBACK");
        ImGui::Separator();

        if (ImGui::Button("Run Benchmark", ImVec2(200, 0))) {
            run_full_benchmark();
        }

        if (is_benchmarking) {
            ImGui::ProgressBar(benchmark_progress, ImVec2(-1, 0));
            ImGui::Text("Benchmarking... %.0f%%", benchmark_progress * 100);
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    }
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

#if defined(ARRAY_USE_GLES)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(1600, 1000, "NEON Performance Benchmark", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.08f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.10f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.15f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.20f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.20f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.25f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.20f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.25f, 0.45f, 1.0f);

    style.WindowPadding = ImVec2(15, 15);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 1.8f;

    PerformanceBenchmark benchmark;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
        ImGui::Begin("NEON Performance Benchmark", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        benchmark.draw_controls();
        ImGui::Separator();

        if (ImGui::BeginTable("Layout", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Graphs", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Results", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            benchmark.draw_combined_graph();
            ImGui::Spacing();
            benchmark.draw_speedup_chart();

            ImGui::TableNextColumn();
            ImGui::Text("Benchmark Results");
            ImGui::Separator();
            benchmark.draw_table();

            ImGui::EndTable();
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.05f, 0.08f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}