cmake_minimum_required(VERSION 3.16)

project(array_processing_arm_neon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_ARRAY_PROCESSING "Build shared library target" ON)

add_library(array_processing_static STATIC src/array_sum.cpp)
set_target_properties(array_processing_static PROPERTIES OUTPUT_NAME array_processing)
target_include_directories(array_processing_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(array_processing_static PRIVATE -O3 -Wall -Wextra -Wpedantic)

if(BUILD_SHARED_ARRAY_PROCESSING)
    add_library(array_processing_shared SHARED src/array_sum.cpp)
    set_target_properties(array_processing_shared PROPERTIES OUTPUT_NAME array_processing)
    target_include_directories(array_processing_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(array_processing_shared PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7.*)$" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # 32-bit ARM GNU toolchains usually need explicit NEON + hard-float flags.
    target_compile_options(array_processing_static PRIVATE -mfpu=neon -mfloat-abi=hard)
    if(TARGET array_processing_shared)
        target_compile_options(array_processing_shared PRIVATE -mfpu=neon -mfloat-abi=hard)
    endif()
endif()

add_executable(go src/main.cpp)
target_link_libraries(go PRIVATE array_processing_static)

# Keep executable at project root for qemu-arm command:
# qemu-arm -L /usr/arm-linux-gnueabihf ./go
set_target_properties(go PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
