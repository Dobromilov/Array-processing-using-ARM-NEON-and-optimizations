CMAKE ?= cmake
CMAKE_ARGS ?=
BUILD_DIR ?= build-host
BUILD_DIR_ARMHF ?= build-armhf
BUILD_DIR_ARMHF_CLI ?= build-armhf-cli
BUILD_TYPE ?= Release
TARGET ?= array_benchmark
CLI_TARGET ?= array_benchmark_cli
JOBS ?= $(shell nproc 2>/dev/null || echo 4)
TOOLCHAIN_ARMHF ?= cmake/toolchains/armhf.cmake
QEMU_SYSROOT ?= /usr/arm-linux-gnueabihf
ARM_RUNNER ?= scripts/run-armhf.sh

.PHONY: all configure build run host-configure host-build host-run \
	armhf-configure armhf-build armhf-run armhf \
	armhf-cli-configure armhf-cli-build armhf-cli-run \
	run-armhf run-armhf-cli clean distclean info help

all: build

configure: host-configure

build: host-build

run: host-run

host-configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_ARGS)

host-build: host-configure
	$(CMAKE) --build $(BUILD_DIR) --target $(TARGET) -j$(JOBS)

host-run: host-build
	./$(BUILD_DIR)/$(TARGET)

armhf-configure:
	@if [ -f $(BUILD_DIR_ARMHF)/CMakeCache.txt ]; then \
		$(CMAKE) -S . -B $(BUILD_DIR_ARMHF) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DARRAY_BUILD_GUI=ON \
			-DARRAY_BUILD_CLI=ON \
			$(CMAKE_ARGS); \
	else \
		$(CMAKE) -S . -B $(BUILD_DIR_ARMHF) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_ARMHF) \
			-DARRAY_BUILD_GUI=ON \
			-DARRAY_BUILD_CLI=ON \
			$(CMAKE_ARGS); \
	fi

armhf-build: armhf-configure
	$(CMAKE) --build $(BUILD_DIR_ARMHF) --target $(TARGET) -j$(JOBS)

armhf-run: armhf-build
	QEMU_SYSROOT=$(QEMU_SYSROOT) $(ARM_RUNNER) $(BUILD_DIR_ARMHF)/$(TARGET)

armhf: armhf-build

armhf-cli-configure:
	@if [ -f $(BUILD_DIR_ARMHF_CLI)/CMakeCache.txt ]; then \
		$(CMAKE) -S . -B $(BUILD_DIR_ARMHF_CLI) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DARRAY_BUILD_GUI=OFF \
			-DARRAY_BUILD_CLI=ON \
			$(CMAKE_ARGS); \
	else \
		$(CMAKE) -S . -B $(BUILD_DIR_ARMHF_CLI) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_ARMHF) \
			-DARRAY_BUILD_GUI=OFF \
			-DARRAY_BUILD_CLI=ON \
			$(CMAKE_ARGS); \
	fi

armhf-cli-build: armhf-cli-configure
	$(CMAKE) --build $(BUILD_DIR_ARMHF_CLI) --target $(CLI_TARGET) -j$(JOBS)

armhf-cli-run: armhf-cli-build
	QEMU_SYSROOT=$(QEMU_SYSROOT) $(ARM_RUNNER) $(BUILD_DIR_ARMHF_CLI)/$(CLI_TARGET)

run-armhf: armhf-run

run-armhf-cli: armhf-cli-run

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean 2>/dev/null || true
	$(CMAKE) --build $(BUILD_DIR_ARMHF) --target clean 2>/dev/null || true
	$(CMAKE) --build $(BUILD_DIR_ARMHF_CLI) --target clean 2>/dev/null || true

distclean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR_ARMHF) $(BUILD_DIR_ARMHF_CLI)

info:
	@echo "BUILD_DIR=$(BUILD_DIR)"
	@echo "CMAKE_ARGS=$(CMAKE_ARGS)"
	@echo "BUILD_DIR_ARMHF=$(BUILD_DIR_ARMHF)"
	@echo "BUILD_DIR_ARMHF_CLI=$(BUILD_DIR_ARMHF_CLI)"
	@echo "BUILD_TYPE=$(BUILD_TYPE)"
	@echo "TARGET=$(TARGET)"
	@echo "CLI_TARGET=$(CLI_TARGET)"
	@echo "TOOLCHAIN_ARMHF=$(TOOLCHAIN_ARMHF)"
	@echo "QEMU_SYSROOT=$(QEMU_SYSROOT)"

help:
	@echo "Available commands:"
	@echo "  make host-run        Build and run the GUI for the current machine"
	@echo "  make armhf-run       Cross-build and run the ARM GUI via qemu-arm"
	@echo "  make armhf-cli-run   Cross-build and run the ARM console smoke benchmark"
	@echo "  make clean           Clean configured build directories"
	@echo "  make distclean       Remove build directories"
