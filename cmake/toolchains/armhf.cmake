set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_C_FLAGS_INIT "-mfpu=neon -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "-mfpu=neon -mfloat-abi=hard")

# Debian/Ubuntu multiarch layout for armhf cross builds.
set(CMAKE_FIND_ROOT_PATH
    /usr/arm-linux-gnueabihf
    /usr/lib/arm-linux-gnueabihf
    /lib/arm-linux-gnueabihf
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Keep pkg-config on target (armhf) .pc files.
set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "/")
set(ENV{PKG_CONFIG_LIBDIR}
    "/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/share/pkgconfig")
