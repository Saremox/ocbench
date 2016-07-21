CHECK_INCLUDE_FILES(sqlite3.h HAVE_SQLITE3_H)

CHECK_LIBRARY_EXISTS(sqlite3 sqlite3_libversion "" HAVE_SQLITE3)

add_library(ocdata ocdata.c)
IF(HAVE_SQLITE3_H AND HAVE_SQLITE3)
  target_link_libraries(ocdata -lsqlite3)
ELSE()

ENDIF()
