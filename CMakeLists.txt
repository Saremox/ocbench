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

include_directories("${PROJECT_SOURCE_DIR}/src"
                    "${PROJECT_BINARY_DIR}/src"
                    "${PROJECT_BINARY_DIR}/include")

add_subdirectory(src)
add_subdirectory(test)
