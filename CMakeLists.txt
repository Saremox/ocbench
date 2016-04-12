cmake_minimum_required(VERSION 2.6)
project(openCompressBench)

set (OCBENCH_VERSION_MAJOR 0)
set (OCBENCH_VERSION_MINOR 1)

configure_file(
  "${PROJECT_SOURCE_DIR}/ocbenchConfig.h.in"
  "${PROJECT_BINARY_DIR}/ocbenchConfig.h"
)

include_directories("${PROJECT_BINARY_DIR}")

add_executable(ocBench ocbench.c)
