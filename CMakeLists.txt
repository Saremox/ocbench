cmake_minimum_required(VERSION 2.6)
project(openCompressBench)

set (OCBENCH_VERSION_MAJOR 0)
set (OCBENCH_VERSION_MINOR 1)

IF(BUILD_SQUASH)
  include(ExternalProject)

  ExternalProject_Add(squash
    PREFIX "${PROJECT_BINARY_DIR}/squash"
    GIT_REPOSITORY "https://github.com/quixdb/squash.git"
    GIT_SUBMODULES ""
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:PATH=${PROJECT_BINARY_DIR}"
  )
ENDIF()

include_directories("${PROJECT_SOURCE_DIR}/src"
                    "${PROJECT_BINARY_DIR}/src"
                    "${PROJECT_BINARY_DIR}/include")

add_subdirectory(src)
add_subdirectory(test)
