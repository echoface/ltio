
find_path(MYSQL_INCLUDE_DIRS mysql.h
  PATHS
  PATH_SUFFIXES include mysql)
find_library(MYSQL_LIBRARIES mariadbclient
  PATHS mysql
  PATH_SUFFIXES lib lib64 mysql)

if (MYSQL_LIBRARIES)
else ()
  find_path(MYSQL_INCLUDE_DIRS mysql.h
    PATHS
    PATH_SUFFIXES include mysql)
  find_library(MYSQL_LIBRARIES mysqlclient
    PATHS mysql
    PATH_SUFFIXES lib lib64 mysql)
endif()

message(STATUS "mysql include: " ${MYSQL_INCLUDE_DIRS})
message(STATUS "mysql library: " ${MYSQL_LIBRARIES})
