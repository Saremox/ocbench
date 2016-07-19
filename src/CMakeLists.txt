add_subdirectory(ocmemfd)
add_subdirectory(ocsched)

configure_file(
  "ocbenchConfig.h.in"
  "ocbenchConfig.h"
)

add_executable(ocBench ocbench.c)
target_link_libraries(ocBench ocmemfd ocsched ${Squash_LIBRARIES})
target_include_directories(ocBench PUBLIC ${Squash_INCLUDE_DIRS})
