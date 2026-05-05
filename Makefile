# NEON Performance Benchmark Makefile for Raspberry Pi
CXX = g++
CXXFLAGS = -std=c++17 -O3 -ftree-vectorize -ffast-math
LDFLAGS = -lglfw -lGL -lpthread

# Определение архитектуры ARM
ARCH := $(shell uname -m)
ifeq ($(ARCH), armv7l)
    CXXFLAGS += -mfpu=neon -march=armv7-a -mtune=cortex-a7 -DARM_NEON_ENABLED=1
    $(info Building for ARMv7 with NEON support)
else ifeq ($(ARCH), aarch64)
    CXXFLAGS += -march=armv8-a+simd -DARM_NEON_ENABLED=1
    $(info Building for ARM64 with NEON support)
else ifeq ($(ARCH), armv6l)
    CXXFLAGS += -mfpu=vfp -march=armv6 -DARM_NEON_ENABLED=0
    $(warning NEON not supported on ARMv6)
else
    CXXFLAGS += -DARM_NEON_ENABLED=0
    $(warning Building for non-ARM architecture)
endif

# Директории
BUILD_DIR = build
DEPS_DIR = deps
BIN_DIR = bin

# Исходные файлы
SOURCES = main.cpp
IMGUI_SOURCES = $(DEPS_DIR)/imgui/imgui.cpp \
                $(DEPS_DIR)/imgui/imgui_draw.cpp \
                $(DEPS_DIR)/imgui/imgui_tables.cpp \
                $(DEPS_DIR)/imgui/imgui_widgets.cpp \
                $(DEPS_DIR)/imgui/backends/imgui_impl_glfw.cpp \
                $(DEPS_DIR)/imgui/backends/imgui_impl_opengl3.cpp

HEADERS = array_sum.h

# Объектные файлы
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.cpp=.o))) \
          $(addprefix $(BUILD_DIR)/, $(notdir $(IMGUI_SOURCES:.cpp=.o)))

# Исполняемый файл
TARGET = $(BIN_DIR)/neon_benchmark

# Флаги для отладки
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -O0
    $(info Debug build enabled)
else
    CXXFLAGS += -s
endif

# Цветной вывод
GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[1;33m
BLUE = \033[0;34m
NC = \033[0m

.PHONY: all clean run deps help debug

all: deps $(TARGET)

# Проверка и установка зависимостей
deps:
	@echo -e "$(BLUE)Checking dependencies...$(NC)"
	@if ! command -v g++ >/dev/null 2>&1; then \
		echo -e "$(RED)Error: g++ not found. Installing...$(NC)"; \
		sudo apt-get update && sudo apt-get install -y g++; \
	fi
	@if ! ldconfig -p | grep -q libglfw; then \
		echo -e "$(YELLOW)GLFW not found. Installing...$(NC)"; \
		sudo apt-get install -y libglfw3-dev; \
	fi
	@if ! ldconfig -p | grep -q libGL; then \
		echo -e "$(YELLOW)OpenGL not found. Installing...$(NC)"; \
		sudo apt-get install -y libgl1-mesa-dev; \
	fi
	@echo -e "$(GREEN)✓ Dependencies satisfied$(NC)"
	@mkdir -p $(DEPS_DIR) $(BUILD_DIR) $(BIN_DIR)
	@$(MAKE) --no-print-directory download-deps

download-deps:
	@# Download ImGui if not exists
	@if [ ! -d "$(DEPS_DIR)/imgui" ]; then \
		echo -e "$(BLUE)Downloading ImGui...$(NC)"; \
		git clone --depth 1 --branch v1.90.6 https://github.com/ocornut/imgui.git $(DEPS_DIR)/imgui; \
		echo -e "$(GREEN)✓ ImGui downloaded$(NC)"; \
	fi

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@echo -e "$(BLUE)Compiling $<...$(NC)"
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(DEPS_DIR)/imgui -I$(DEPS_DIR)/imgui/backends -c $< -o $@

$(BUILD_DIR)/%.o: $(DEPS_DIR)/imgui/%.cpp
	@echo -e "$(BLUE)Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) -I$(DEPS_DIR)/imgui -I$(DEPS_DIR)/imgui/backends -c $< -o $@

$(BUILD_DIR)/%.o: $(DEPS_DIR)/imgui/backends/%.cpp
	@echo -e "$(BLUE)Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) -I$(DEPS_DIR)/imgui -I$(DEPS_DIR)/imgui/backends -c $< -o $@

$(TARGET): $(OBJECTS)
	@echo -e "$(BLUE)Linking...$(NC)"
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo -e "$(GREEN)✓ Build complete!$(NC)"
	@echo -e "$(GREEN)Executable: $(TARGET)$(NC)"
	@ls -lh $(TARGET)

run: $(TARGET)
	@echo -e "$(BLUE)Running NEON Benchmark...$(NC)"
	@echo -e "$(YELLOW)========================================$(NC)"
	./$(TARGET)

clean:
	@echo -e "$(YELLOW)Cleaning build files...$(NC)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo -e "$(GREEN)✓ Clean complete$(NC)"

clean-deps:
	@echo -e "$(YELLOW)Removing dependencies...$(NC)"
	@rm -rf $(DEPS_DIR)
	@echo -e "$(GREEN)✓ Dependencies removed$(NC)"

distclean: clean clean-deps
	@echo -e "$(GREEN)✓ Full clean complete$(NC)"

debug:
	@$(MAKE) DEBUG=1 all

info:
	@echo -e "$(BLUE)Build Information:$(NC)"
	@echo -e "Architecture: $(ARCH)"
	@echo -e "Compiler: $(CXX)"
	@echo -e "Flags: $(CXXFLAGS)"
	@echo -e "Target: $(TARGET)"

help:
	@echo -e "$(BLUE)Available commands:$(NC)"
	@echo -e "  $(GREEN)make deps$(NC)    - Install required dependencies"
	@echo -e "  $(GREEN)make all$(NC)     - Build the project (default)"
	@echo -e "  $(GREEN)make run$(NC)     - Build and run the benchmark"
	@echo -e "  $(GREEN)make clean$(NC)   - Remove build files"
	@echo -e "  $(GREEN)make debug$(NC)   - Build with debug symbols"
	@echo -e "  $(GREEN)make info$(NC)    - Show build configuration"
	@echo -e "  $(GREEN)make help$(NC)    - Show this help"
	@echo ""
	@echo -e "$(YELLOW)Quick start:$(NC)"
	@echo -e "  $(GREEN)make deps && make run$(NC)"