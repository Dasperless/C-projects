cmake_minimum_required(VERSION 3.20)

# Out-of-source build guard
file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}" current_bin_dir)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" current_src_dir)
if(current_bin_dir STREQUAL current_src_dir)
    message(FATAL_ERROR "In-source builds are not permitted. Please use a separate build directory (e.g. cmake -B build).")
endif()

project(CProjects VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Binary and library output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build options
option(ENABLE_WERROR "Treat compiler warnings as errors" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Interface library for compiler warnings and sanitizers
add_library(project_warnings INTERFACE)

target_compile_options(project_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    $<$<BOOL:${ENABLE_WERROR}>:-Werror>
)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ENABLE_ASAN and ENABLE_TSAN cannot be enabled simultaneously.")
endif()

if(ENABLE_ASAN)
    target_compile_options(project_warnings INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(project_warnings INTERFACE -fsanitize=address)
endif()

if(ENABLE_UBSAN)
    target_compile_options(project_warnings INTERFACE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(project_warnings INTERFACE -fsanitize=undefined)
endif()

if(ENABLE_TSAN)
    target_compile_options(project_warnings INTERFACE -fsanitize=thread)
    target_link_options(project_warnings INTERFACE -fsanitize=thread)
endif()

# Threading support
find_package(Threads REQUIRED)

# Testing
include(CTest)

# Subdirectories
add_subdirectory(chatroom)
add_subdirectory(structures)
