cmake_minimum_required(VERSION 3.16)

project(array_processing_arm_neon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Try multiple ways to find GLFW
if(NOT GLFW_FOUND)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW_PKG glfw3)
        if(GLFW_PKG_FOUND)
            set(GLFW_INCLUDE_DIRS ${GLFW_PKG_INCLUDE_DIRS})
            set(GLFW_LIBRARIES ${GLFW_PKG_LIBRARIES})
            set(GLFW_FOUND TRUE)
            message(STATUS "Found GLFW via pkg-config: ${GLFW_LIBRARIES}")
        endif()
    endif()

    if(NOT GLFW_FOUND)
        find_library(GLFW_LIBRARY
                NAMES glfw glfw3
                PATHS /usr/lib /usr/lib/x86_64-linux-gnu /lib /lib/x86_64-linux-gnu
                /usr/arm-linux-gnueabihf/lib
                REQUIRED
        )
        if(GLFW_LIBRARY)
            set(GLFW_LIBRARIES ${GLFW_LIBRARY})
            set(GLFW_FOUND TRUE)
            set(GLFW_INCLUDE_DIRS /usr/include)
            message(STATUS "Found GLFW library: ${GLFW_LIBRARY}")
        endif()
    endif()
endif()

if(NOT GLFW_FOUND)
    message(FATAL_ERROR "GLFW not found. Please install: sudo apt-get install libglfw3-dev")
endif()

# Download ImGui
include(FetchContent)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.8
)
FetchContent_MakeAvailable(imgui)

# Collect ImGui sources
set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Library
add_library(array_processing STATIC src/array_sum.cpp)
target_include_directories(array_processing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(array_processing PRIVATE -O3 -Wall -Wextra -Wpedantic)

# NEON optimizations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64|arm64")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(array_processing PRIVATE -march=armv8-a+fp+simd)
        message(STATUS "NEON optimizations enabled")
    endif()
else()
    message(STATUS "NEON optimizations disabled (non-ARM platform)")
endif()

# Main executable
add_executable(array_benchmark src/main.cpp ${IMGUI_SOURCES})

# Include directories
target_include_directories(array_benchmark PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${GLFW_INCLUDE_DIRS}
)

# Compile definitions
target_compile_definitions(array_benchmark PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3)

# Link libraries
target_link_libraries(array_benchmark PRIVATE
        array_processing
        ${OPENGL_LIBRARIES}
        ${GLFW_LIBRARIES}
)

# For Linux/ARM cross-compilation
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(array_benchmark PRIVATE
            pthread
            dl
            X11
            Xrandr
            Xi
            Xxf86vm
            Xcursor
            Xinerama
    )
endif()