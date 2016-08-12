cmake_minimum_required(VERSION 2.6)
project(openCompressBench)

set (OCBENCH_VERSION_MAJOR 0)
set (OCBENCH_VERSION_MINOR 1)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckSymbolExists)
INCLUDE (CheckLibraryExists)
INCLUDE (CheckTypeSize)

find_package(Squash)

set(CMAKE_C_FLAGS_DEBUG "-Og -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE")
set(CMAKE_C_FLAGS_RELEASE "-O3 -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE")

include_directories("${PROJECT_SOURCE_DIR}/src"
                    "${PROJECT_BINARY_DIR}/src"
                    "${PROJECT_BINARY_DIR}/include")

add_subdirectory(src)
add_subdirectory(test)
