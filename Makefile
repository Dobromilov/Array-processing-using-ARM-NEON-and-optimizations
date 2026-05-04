CMAKE ?= cmake
BUILD_DIR ?= build-arm
HOST_BUILD_DIR ?= build-host
BUILD_TYPE ?= Release
TOOLCHAIN_FILE ?= cmake/toolchains/armhf.cmake
QEMU_SYSROOT ?= /usr/arm-linux-gnueabihf

.PHONY: all configure build run host-configure host-build host-run clean rebuild help

all: build

configure:
	@echo "Configuring for ARM cross-compilation..."
	@mkdir -p $(BUILD_DIR)
	if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE); \
	else \
		$(CMAKE) -S . -B $(BUILD_DIR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE); \
	fi

build: configure
	@echo "Building ARM executable..."
	$(CMAKE) --build $(BUILD_DIR) --parallel
	@cp $(BUILD_DIR)/array_benchmark ./array_benchmark_arm

run: build
	@echo "Running ARM NEON benchmark with QEMU..."
	@if [ -f "./array_benchmark_arm" ]; then \
		qemu-arm -L $(QEMU_SYSROOT) ./array_benchmark_arm; \
	else \
		echo "ARM executable not found. Please run 'make build' first."; \
		exit 1; \
	fi

host-configure:
	@echo "Configuring for host compilation..."
	@mkdir -p $(HOST_BUILD_DIR)
	$(CMAKE) -S . -B $(HOST_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

host-build: host-configure
	@echo "Building host executable..."
	$(CMAKE) --build $(HOST_BUILD_DIR) --parallel
	@cp $(HOST_BUILD_DIR)/array_benchmark ./array_benchmark_host

host-run: host-build
	@echo "Running host benchmark..."
	@if [ -f "./array_benchmark_host" ]; then \
		./array_benchmark_host; \
	else \
		echo "Host executable not found. Please run 'make host-build' first."; \
		exit 1; \
	fi

clean:
	@echo "Cleaning build directories..."
	rm -rf $(BUILD_DIR) $(HOST_BUILD_DIR)
	@echo "Cleaning executables..."
	rm -f array_benchmark_arm array_benchmark_host
	@echo "Clean complete!"

rebuild: clean build

help:
	@echo "Available commands:"
	@echo "  make build        - Build ARM executable (cross-compile)"
	@echo "  make run          - Run ARM executable with QEMU"
	@echo "  make host-build   - Build native host executable"
	@echo "  make host-run     - Run native host executable"
	@echo "  make clean        - Clean build files and executables"
	@echo "  make rebuild      - Clean and rebuild ARM executable"
	@echo "  make help         - Show this help message"