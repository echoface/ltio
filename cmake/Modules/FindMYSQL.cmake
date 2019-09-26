# Find MySQL

# Find the native MySQL includes and library
#
#  MYSQL_FOUND       - True if MySQL found.
#  MYSQL_LIBRARIES   - List of libraries when using MySQL.
#  MYSQL_INCLUDE_DIR - where to find mysql.h, etc.

if(NOT WIN32)
  find_program(MYSQL_CONFIG_EXECUTABLE mysql_config
    /usr/bin/
    /usr/local/bin
    $ENV{MYSQL_DIR}/bin
  )
endif()

set(MYSQL_NAMES mariadbclient mysqlclient mysqlclient_r)

if(MYSQL_CONFIG_EXECUTABLE)
  execute_process(COMMAND ${MYSQL_CONFIG_EXECUTABLE} --cflags OUTPUT_VARIABLE MYSQL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(MYSQL_CFLAGS)
  string( REGEX MATCH "-I[^;]+" MYSQL_INCLUDE_DIR "${MYSQL_CFLAGS}" )
  string( REPLACE "-I" "" MYSQL_INCLUDE_DIR "${MYSQL_INCLUDE_DIR}")
  string( REGEX REPLACE "-I[^;]+;" "" MYSQL_CFLAGS "${MYSQL_CFLAGS}" )
  execute_process(COMMAND ${MYSQL_CONFIG_EXECUTABLE} --libs OUTPUT_VARIABLE MYSQL_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS
    /usr/include
    /usr/local/include
    /usr/include/mysql
    /usr/include/mariadb
    /usr/mysql/include
    $ENV{MYSQL_DIR}/include
    /usr/local/mysql/include
    /usr/local/include/mysql
    PATH_SUFFIXES include 
  )
  find_library(MYSQL_LIBRARIES NAMES ${MYSQL_NAMES}
    PATHS
    /usr/lib
    /usr/local/lib
    /usr/local/mysql/lib
    $ENV{MYSQL_DIR}/lib
    $ENV{MYSQL_DIR}/lib/opt
    PATH_SUFFIXES lib lib64 mysql mariadb
  )
endif()

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
  if(WIN32)
    set(MYSQL_LIBRARY ${MYSQL_LIBRARIES})
    set(MYSQL_INCLUDE_DIR ${MYSQL_INCLUDE_DIR})
    string(REPLACE mysqlclient libmysql libmysql ${MYSQL_LIBRARY})
  endif()
else()
  set(MYSQL_LIBRARY )
  set(MYSQL_INCLUDE_DIR )
endif()

find_package_handle_standard_args(MYSQL DEFAULT_MSG MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)
if(MYSQL_FOUND)
  if(NOT MYSQL_FIND_QUIETLY)
    message(STATUS "Found MySQL: (${MYSQL_INCLUDE_DIR} ${MYSQL_LIBRARIES})")
  endif()
else()
  message(STATUS "package MySQL named ${MYSQL_NAMES} NOT FOUND")
  if(MYSQL_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find MySQL library")
  endif()
endif()

mark_as_advanced(
  MYSQL_LIBRARY
  MYSQL_INCLUDE_DIR
  MYSQL_CONFIG_EXECUTABLE
)
