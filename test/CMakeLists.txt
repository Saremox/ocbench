add_executable(Testocmemfd Testocmemfd.c)
add_executable(Testocunit Testocunit.c)
add_executable(Testocsched Testocsched.c)
target_link_libraries(Testocmemfd ocmemfd)
target_link_libraries(Testocsched ocsched)

add_test(NAME ocmemfd
COMMAND $<TARGET_FILE:Testocmemfd>)

add_test(NAME ocunit
COMMAND $<TARGET_FILE:Testocunit>)

add_test(NAME ocsched
COMMAND $<TARGET_FILE:Testocsched>)
