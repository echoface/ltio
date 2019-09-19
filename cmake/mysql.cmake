
find_path(MYSQL_INCLUDE_DIRS mysql.h
  PATHS
  PATH_SUFFIXES include)
find_library(MYSQL_LIBRARIES mariadbclient
  PATHS
  PATH_SUFFIXES lib lib64)

if (MYSQL_LIBRARIES)
else ()
  find_path(MYSQL_INCLUDE_DIRS mysql.h
    PATHS
    PATH_SUFFIXES include)
  find_library(MYSQL_LIBRARIES mysqlclient
    PATHS
    PATH_SUFFIXES lib lib64)
endif()

message(STATUS "mysql include: " ${MYSQL_INCLUDE_DIRS})
message(STATUS "mysql library: " ${MYSQL_LIBRARIES})
