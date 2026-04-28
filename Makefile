CMAKE ?= cmake
BUILD_DIR ?= build-arm
HOST_BUILD_DIR ?= build-host
BUILD_TYPE ?= Release
TOOLCHAIN_FILE ?= cmake/toolchains/armhf.cmake
QEMU_SYSROOT ?= /usr/arm-linux-gnueabihf

.PHONY: all configure build run host-configure host-build host-run native-arm-build native-arm-run clean rebuild

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

native-arm-build:
	arch=$$(uname -m); \
	if [ "$$arch" = "arm64" ] || [ "$$arch" = "aarch64" ]; then \
		$(MAKE) host-build; \
	else \
		echo "native-arm-build is only for ARM hosts (arm64/aarch64). Current: $$arch"; \
		echo "On Apple Silicon, run a native terminal, not a Rosetta terminal."; \
		exit 1; \
	fi

native-arm-run: native-arm-build
	./go

clean:
	rm -rf $(BUILD_DIR) $(HOST_BUILD_DIR) go

rebuild: clean build
