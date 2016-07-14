cmake_minimum_required(VERSION 2.6)
project(openCompressBench)

set (OCBENCH_VERSION_MAJOR 0)
set (OCBENCH_VERSION_MINOR 1)

include_directories("${PROJECT_SOURCE_DIR}/src"
                    "${PROJECT_BINARY_DIR}/src")

add_subdirectory(src)
enable_testing()
add_subdirectory(test)
