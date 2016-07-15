add_executable(Testocmemfd Testocmemfd.c)
add_executable(Testocunit Testocunit.c)
add_executable(Testocsched Testocsched.c)
target_link_libraries(Testocmemfd ocmemfd)
target_link_libraries(Testocsched ocsched)

add_custom_target("test"
COMMAND "${PROJECT_SOURCE_DIR}/test/runtests.sh")
