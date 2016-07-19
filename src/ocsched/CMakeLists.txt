add_library(ocsched ocsched.c)
target_link_libraries(ocsched rt ${Squash_LIBRARIES})
target_include_directories(ocsched PUBLIC ${Squash_INCLUDE_DIRS})
