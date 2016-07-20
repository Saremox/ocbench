cmake_policy(PUSH)

IF(CMAKE_VERSION VERSION_GREATER "3.0")
cmake_policy(SET CMP0037 OLD)
ENDIF()

add_executable(Testocmemfd Testocmemfd.c)
add_executable(Testocunit Testocunit.c)
add_executable(Testocsched Testocsched.c)
target_link_libraries(Testocmemfd ocmemfd)
target_link_libraries(Testocsched ocsched)

add_custom_target("test"
COMMAND "${PROJECT_SOURCE_DIR}/test/runtests.sh")

cmake_policy(POP)
