add_subdirectory(ocmemfd)

configure_file(
  "ocbenchConfig.h.in"
  "ocbenchConfig.h"
)

add_executable(ocBench ocbench.c)
target_link_libraries(ocBench ocmemfd)
