ADD_DEFINITIONS(-std=c++11)

aux_source_directory(. ASYNC_MYSQL_SOURCES)

add_library(ltmysql STATIC ${ASYNC_MYSQL_SOURCES})

ltio_default_properties(ltmysql)
set_target_properties(ltmysql PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_include_directories(ltmysql PUBLIC
  ${LtIO_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${MYSQL_INCLUDE_DIR}/mysql
  ${MYSQL_INCLUDE_DIR}/mariadb
  ${PROJECT_SOURCE_DIR}/thirdparty
  )

#ADD_EXECUTABLE(async_mysql_test
#  test/async_main.cc
#)
#
#TARGET_LINK_LIBRARIES(async_mysql_test
#  ${LtIO_LINKER_LIBS}
#  ltbase
#)
#set_target_properties(async_mysql_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
