find_package(Sqlite3 REQUIRED)

add_library(ocdata ocdata.c)
target_link_libraries(ocdata ${SQLITE3_LIBRARIES} ocutils)
target_include_directories(ocdata PUBLIC ${SQLITE3_INCLUDE_DIR})
