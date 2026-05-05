cmake_minimum_required(VERSION 3.16)

project(array_processing_arm_neon LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ARRAY_BUILD_GUI "Build the ImGui/GLFW benchmark window" ON)
option(ARRAY_BUILD_CLI "Build a console benchmark for ARM/QEMU smoke runs" ON)
option(ARRAY_FETCH_IMGUI "Download ImGui if vendors/imgui is not present" ON)
option(ARRAY_FETCH_GLFW "Download GLFW if a system glfw3 package is not present" ON)
option(ARRAY_USE_GLES "Build the GUI against OpenGL ES 2 instead of desktop OpenGL" OFF)

add_library(array_processing STATIC src/array_sum.cpp)
target_include_directories(array_processing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(array_processing PRIVATE -O3 -Wall -Wextra -Wpedantic)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        target_compile_options(array_processing PRIVATE -march=armv8-a+simd)
        target_compile_definitions(array_processing PUBLIC ARM_NEON_ENABLED=1)
        message(STATUS "NEON flags enabled for ARM64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(armv7l|armv7|arm)$")
        target_compile_options(array_processing PRIVATE -mfpu=neon -march=armv7-a -mfloat-abi=hard)
        target_compile_definitions(array_processing PUBLIC ARM_NEON_ENABLED=1)
        message(STATUS "NEON flags enabled for ARMv7")
    else()
        target_compile_definitions(array_processing PUBLIC ARM_NEON_ENABLED=0)
        message(STATUS "NEON flags disabled for non-ARM processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

if(ARRAY_BUILD_CLI)
    add_executable(array_benchmark_cli src/cli_main.cpp)
    target_link_libraries(array_benchmark_cli PRIVATE array_processing)
endif()

if(ARRAY_BUILD_GUI)
    include(FetchContent)

    find_package(Threads REQUIRED)

    if(ARRAY_USE_GLES)
        find_package(OpenGL QUIET COMPONENTS EGL GLES2)
        if(NOT OpenGL_FOUND AND NOT OPENGL_FOUND)
            message(FATAL_ERROR
                "OpenGL ES libraries for the target architecture were not found. "
                "For armhf cross builds install libgles2-mesa-dev:armhf and libegl1-mesa-dev:armhf, "
                "or configure with -DARRAY_USE_GLES=OFF to use desktop OpenGL.")
        endif()
        set(ARRAY_OPENGL_TARGETS OpenGL::GLES2 OpenGL::EGL)
        message(STATUS "Using OpenGL ES 2 backend")
    else()
        if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
            # For armhf cross builds prefer libGL over GLVND OpenGL+GLX pair.
            set(OpenGL_GL_PREFERENCE LEGACY)
        endif()

        find_package(OpenGL QUIET)
        if(NOT OpenGL_FOUND AND NOT OPENGL_FOUND)
            if(CMAKE_CROSSCOMPILING)
                message(FATAL_ERROR
                    "OpenGL libraries for the target architecture were not found. "
                    "For armhf cross builds install libgl1-mesa-dev:armhf, or run the GUI build on the ARM device.")
            else()
                message(FATAL_ERROR
                    "OpenGL libraries were not found. Install libgl1-mesa-dev or the equivalent OpenGL dev package.")
            endif()
        endif()
        if(TARGET OpenGL::GL)
            set(ARRAY_OPENGL_TARGETS OpenGL::GL)
        else()
            set(ARRAY_OPENGL_TARGETS ${OPENGL_LIBRARIES})
        endif()
        message(STATUS "Using desktop OpenGL backend")
    endif()

    set(ARRAY_GLFW_TARGET "")
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW3 QUIET IMPORTED_TARGET glfw3)
        if(GLFW3_FOUND)
            set(ARRAY_GLFW_TARGET PkgConfig::GLFW3)
            message(STATUS "Using GLFW from pkg-config")
        endif()
    endif()

    if(NOT ARRAY_GLFW_TARGET)
        find_package(glfw3 QUIET)
        if(TARGET glfw)
            set(ARRAY_GLFW_TARGET glfw)
            message(STATUS "Using GLFW CMake package")
        endif()
    endif()

    if(NOT ARRAY_GLFW_TARGET)
        if(NOT ARRAY_FETCH_GLFW)
            message(FATAL_ERROR "GLFW not found and ARRAY_FETCH_GLFW=OFF")
        endif()

        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
        )
        FetchContent_MakeAvailable(glfw)
        set(ARRAY_GLFW_TARGET glfw)
        message(STATUS "Using fetched GLFW")
    endif()

    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendors/imgui")
    if(EXISTS "${IMGUI_DIR}/imgui.cpp")
        message(STATUS "Using vendored ImGui: ${IMGUI_DIR}")
    else()
        if(NOT ARRAY_FETCH_IMGUI)
            message(FATAL_ERROR "ImGui not found in vendors/imgui and ARRAY_FETCH_IMGUI=OFF")
        endif()

        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.90.8
        )
        FetchContent_MakeAvailable(imgui)
        set(IMGUI_DIR "${imgui_SOURCE_DIR}")
        message(STATUS "Using fetched ImGui: ${IMGUI_DIR}")
    endif()

    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC
        ${ARRAY_GLFW_TARGET}
        ${ARRAY_OPENGL_TARGETS}
    )
    target_compile_definitions(imgui PUBLIC
        $<$<BOOL:${ARRAY_USE_GLES}>:IMGUI_IMPL_OPENGL_ES2>
    )

    add_executable(array_benchmark src/main.cpp)
    target_link_libraries(array_benchmark PRIVATE
        array_processing
        imgui
        ${ARRAY_GLFW_TARGET}
        ${ARRAY_OPENGL_TARGETS}
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
    target_compile_definitions(array_benchmark PRIVATE
        $<$<BOOL:${ARRAY_USE_GLES}>:ARRAY_USE_GLES=1>
    )
endif()
