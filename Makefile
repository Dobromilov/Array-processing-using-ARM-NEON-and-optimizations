CXX = g++
CXXFLAGS = -std=c++17 -O3 -ftree-vectorize -ffast-math
LDFLAGS = -lglfw -lGL -lpthread

# Определение архитектуры
ARCH := $(shell uname -m)
ifeq ($(ARCH), aarch64)
    CXXFLAGS += -march=armv8-a+simd -DARM_NEON_ENABLED=1
    $(info ✓ ARM64 with NEON)
else ifeq ($(ARCH), armv7l)
    CXXFLAGS += -mfpu=neon -march=armv7-a -DARM_NEON_ENABLED=1
    $(info ✓ ARMv7 with NEON)
else
    CXXFLAGS += -DARM_NEON_ENABLED=0
    $(info ✗ Non-ARM: $(ARCH))
endif

# Пути
SRC_DIR = src
VENDORS_DIR = vendors
BUILD_DIR = build
BIN_DIR = .

# Исходники
SOURCES = $(SRC_DIR)/main.cpp
IMGUI_DIR = $(VENDORS_DIR)/imgui
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp \
                $(IMGUI_DIR)/imgui_draw.cpp \
                $(IMGUI_DIR)/imgui_tables.cpp \
                $(IMGUI_DIR)/imgui_widgets.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

HEADERS = $(SRC_DIR)/array_sum.h

# Объекты
OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/imgui.o \
          $(BUILD_DIR)/imgui_draw.o \
          $(BUILD_DIR)/imgui_tables.o \
          $(BUILD_DIR)/imgui_widgets.o \
          $(BUILD_DIR)/imgui_impl_glfw.o \
          $(BUILD_DIR)/imgui_impl_opengl3.o

TARGET = $(BIN_DIR)/neon_benchmark

.PHONY: all clean run setup

all: setup $(TARGET)

setup:
	@mkdir -p $(BUILD_DIR)
	@echo "Build directory created"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -c $< -o $@

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -c $< -o $@

$(TARGET): $(OBJECTS)
	@echo "Linking..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Build complete: $(TARGET)"
	@ls -lh $(TARGET)

run: $(TARGET)
	@echo "Running benchmark..."
	@echo "================================"
	./$(TARGET)

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "✓ Clean complete"

info:
	@echo "=== Build Info ==="
	@echo "Arch: $(ARCH)"
	@echo "CXX: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "NEON: $(shell [ $(ARM_NEON_ENABLED) ] && echo Yes || echo No)"