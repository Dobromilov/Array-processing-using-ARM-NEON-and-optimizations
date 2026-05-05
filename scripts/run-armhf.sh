#!/usr/bin/env bash
set -euo pipefail

binary="${1:-build-armhf/array_benchmark}"
sysroot="${QEMU_SYSROOT:-/usr/arm-linux-gnueabihf}"
host_arch="$(uname -m)"

if [[ ! -x "$binary" ]]; then
    echo "ARM executable not found or not executable: $binary" >&2
    echo "Build it first with: make armhf-build" >&2
    exit 1
fi

case "$host_arch" in
    arm*|aarch64)
        exec "$binary"
        ;;
esac

if ! command -v qemu-arm >/dev/null 2>&1; then
    echo "qemu-arm not found. Install qemu-user or run this binary on an ARM machine." >&2
    exit 1
fi

if [[ ! -d "$sysroot" ]]; then
    echo "ARM sysroot not found: $sysroot" >&2
    echo "Set QEMU_SYSROOT to the armhf sysroot path or install armhf runtime libraries." >&2
    exit 1
fi

export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"
exec qemu-arm -L "$sysroot" "$binary"
