
find_path(MYSQL_INCLUDE_DIRS mysql/mysql.h /usr/include)
find_library(MYSQL_LIBRARIES libmysqlclient.so /usr/lib64/mysql/ /usr/lib/x86_64-linux-gnu)

message(STATUS "mysql include: " ${MYSQL_INCLUDE_DIRS})
message(STATUS "mysql library: " ${MYSQL_LIBRARIES})
