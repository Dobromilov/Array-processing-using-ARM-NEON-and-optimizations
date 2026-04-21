CMAKE ?= cmake
BUILD_DIR ?= build-arm
HOST_BUILD_DIR ?= build-host
BUILD_TYPE ?= Release
TOOLCHAIN_FILE ?= cmake/toolchains/armhf.cmake
QEMU_SYSROOT ?= /usr/arm-linux-gnueabihf

.PHONY: all configure build run host-configure host-build host-run clean rebuild

all: build

configure:
	if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE); \
	else \
		$(CMAKE) -S . -B $(BUILD_DIR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE); \
	fi

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

run: build
	qemu-arm -L $(QEMU_SYSROOT) ./go

host-configure:
	$(CMAKE) -S . -B $(HOST_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

host-build: host-configure
	$(CMAKE) --build $(HOST_BUILD_DIR) --parallel

host-run: host-build
	./go

clean:
	rm -rf $(BUILD_DIR) $(HOST_BUILD_DIR) go

rebuild: clean build
