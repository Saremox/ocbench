add_executable(Testocmemfd Testocmemfd.c)
add_executable(Testocunit Testocunit.c)
add_executable(Testocsched Testocsched.c)
add_executable(Testocutils-list Testocutils-list.c)
target_link_libraries(Testocmemfd ocmemfd)
target_link_libraries(Testocsched ocsched)
target_link_libraries(Testocutils-list ocutils)

add_test(NAME ocmemfd
COMMAND $<TARGET_FILE:Testocmemfd>)

add_test(NAME ocunit
COMMAND $<TARGET_FILE:Testocunit>)

add_test(NAME ocsched
COMMAND $<TARGET_FILE:Testocsched>)

add_test(NAME ocutils-list
COMMAND $<TARGET_FILE:Testocutils-list>)
