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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

class PerformanceBenchmark {
private:
    std::vector<int32_t> data;
    std::vector<float> basic_times;
    std::vector<float> neon_times;
    std::vector<int> array_sizes;
    bool is_benchmarking = false;
    float benchmark_progress = 0.0f;

    const std::vector<int> size_options = {
        10, 50, 100, 500, 1000, 5000,
        10000, 50000, 100000, 250000, 500000,
        750000, 1000000, 2500000, 5000000, 10000000
    };

    const std::vector<std::string> size_labels = {
        "10", "50", "100", "500", "1k", "5k",
        "10k", "50k", "100k", "250k", "500k",
        "750k", "1M", "2.5M", "5M", "10M"
    };

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

        for (size_t i = 0; i < size_options.size(); i++) {
            run_single_benchmark(size_options[i]);
            benchmark_progress = static_cast<float>(i + 1) / size_options.size();
        }

        is_benchmarking = false;
    }

    void draw_table() {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        if (ImGui::BeginTable("PerformanceTable", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {

            ImGui::TableSetupColumn("Array Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Basic (us)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("NEON (us)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Speedup", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < array_sizes.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", size_labels[i].c_str());

                ImGui::TableSetColumnIndex(1);
                if (basic_times[i] < 1.0f)
                    ImGui::Text("%.3f", basic_times[i]);
                else
                    ImGui::Text("%.2f", basic_times[i]);

                ImGui::TableSetColumnIndex(2);
                if (neon_times[i] < 1.0f)
                    ImGui::Text("%.3f", neon_times[i]);
                else
                    ImGui::Text("%.2f", neon_times[i]);

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

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        ImGui::Text("Performance: Basic vs NEON");
        ImGui::Spacing();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 graph_pos = ImGui::GetCursorScreenPos();
        ImVec2 graph_size = ImVec2(ImGui::GetContentRegionAvail().x - 20, 400);

        draw_list->AddRectFilled(graph_pos, ImVec2(graph_pos.x + graph_size.x, graph_pos.y + graph_size.y),
                                 IM_COL32(20, 30, 60, 255));
        draw_list->AddRect(graph_pos, ImVec2(graph_pos.x + graph_size.x, graph_pos.y + graph_size.y),
                           IM_COL32(80, 100, 150, 255), 2.0f);

        for (int i = 0; i <= 4; i++) {
            float y = graph_pos.y + (graph_size.y * i / 4.0f);
            draw_list->AddLine(ImVec2(graph_pos.x, y), ImVec2(graph_pos.x + graph_size.x, y),
                               IM_COL32(50, 70, 110, 150), 1.0f);

            float value = max_time * (1.0f - i / 4.0f);
            char y_label[32];
            if (value >= 1000)
                snprintf(y_label, sizeof(y_label), "%.0f ms", value);
            else
                snprintf(y_label, sizeof(y_label), "%.0f us", value);
            draw_list->AddText(ImVec2(graph_pos.x - 50, y - 6), IM_COL32(200, 200, 220, 255), y_label);
        }

        if (basic_times.size() > 1) {
            float x_step = graph_size.x / (basic_times.size() - 1);

            std::vector<ImVec2> basic_points;
            for (size_t i = 0; i < basic_times.size(); i++) {
                float x = graph_pos.x + (i * x_step);
                float y = graph_pos.y + graph_size.y - (basic_times[i] / max_time * graph_size.y);
                basic_points.push_back(ImVec2(x, y));
            }

            for (size_t i = 0; i < basic_points.size() - 1; i++) {
                draw_list->AddLine(basic_points[i], basic_points[i + 1], IM_COL32(100, 150, 255, 255), 3.0f);
            }

            std::vector<ImVec2> neon_points;
            for (size_t i = 0; i < neon_times.size(); i++) {
                float x = graph_pos.x + (i * x_step);
                float y = graph_pos.y + graph_size.y - (neon_times[i] / max_time * graph_size.y);
                neon_points.push_back(ImVec2(x, y));
            }

            for (size_t i = 0; i < neon_points.size() - 1; i++) {
                draw_list->AddLine(neon_points[i], neon_points[i + 1], IM_COL32(100, 255, 100, 255), 3.0f);
            }

            for (size_t i = 0; i < basic_points.size(); i++) {
                if (i >= 6) {
                    draw_list->AddCircleFilled(basic_points[i], 5.0f, IM_COL32(100, 150, 255, 255));
                    draw_list->AddCircle(basic_points[i], 5.0f, IM_COL32(255, 255, 255, 200), 2.0f);
                    draw_list->AddCircleFilled(neon_points[i], 5.0f, IM_COL32(100, 255, 100, 255));
                    draw_list->AddCircle(neon_points[i], 5.0f, IM_COL32(255, 255, 255, 200), 2.0f);
                }
            }
        }

        ImGui::Dummy(graph_size);

        ImVec2 text_pos = ImGui::GetCursorScreenPos();
        float x_step = graph_size.x / (array_sizes.size() - 1);

        for (size_t i = 0; i < array_sizes.size(); i++) {
            if (i % 2 == 0 || i == array_sizes.size() - 1) {
                float x = graph_pos.x + (x_step * i) - 15;
                if (x < graph_pos.x) x = graph_pos.x;
                if (x > graph_pos.x + graph_size.x - 30) x = graph_pos.x + graph_size.x - 30;

                ImGui::GetWindowDrawList()->AddText(ImVec2(x, text_pos.y + 5),
                                                   IM_COL32(200, 200, 220, 255),
                                                   size_labels[i].c_str());
            }
        }

        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "Basic Sum");
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "NEON Optimized");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    }

    void draw_speedup_chart() {
        if (array_sizes.empty()) return;

        std::vector<float> speedups;
        for (size_t i = 0; i < basic_times.size(); i++) {
            speedups.push_back(basic_times[i] / neon_times[i]);
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        ImGui::Text("Speedup Factor");
        ImGui::Spacing();

        float max_speedup = std::max(3.0f, *std::max_element(speedups.begin(), speedups.end()) + 0.5f);
        float available_width = ImGui::GetContentRegionAvail().x - 20;
        float bar_width = (available_width - 40) / speedups.size();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 start_pos = ImGui::GetCursorScreenPos();

        for (size_t i = 0; i < speedups.size(); i++) {
            float bar_height = (speedups[i] / max_speedup) * 200;
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

            if (i < size_labels.size()) {
                ImVec2 label_size = ImGui::CalcTextSize(size_labels[i].c_str());
                draw_list->AddText(ImVec2(bar_start.x + (bar_width - label_size.x) / 2, start_pos.y + 205),
                                  IM_COL32(180, 180, 220, 255), size_labels[i].c_str());
            }
        }

        float baseline_y = start_pos.y + 200 - (1.0f / max_speedup) * 200;
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
    ImGui_ImplOpenGL3_Init("#version 130");

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