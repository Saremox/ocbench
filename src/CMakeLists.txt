add_subdirectory(ocutils)
add_subdirectory(ocdata)
add_subdirectory(ocmemfd)
add_subdirectory(ocsched)

configure_file(
  "ocbenchConfig.h.in"
  "ocbenchConfig.h"
)

find_package (Threads)

add_executable(ocBench ocbench.c ocworker.c job.c)
target_link_libraries(ocBench ocdata ocmemfd
  ocsched ocutils ${Squash_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(ocBench PUBLIC ${Squash_INCLUDE_DIRS})
