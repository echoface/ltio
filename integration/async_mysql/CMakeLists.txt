ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

aux_source_directory(. ASYNC_MYSQL_SOURCES)

ADD_LIBRARY(ltmysql STATIC ${ASYNC_MYSQL_SOURCES})
set_target_properties(ltmysql PROPERTIES EXCLUDE_FROM_ALL TRUE)

ADD_EXECUTABLE(async_mysql_test
  test/async_main.cc
)

TARGET_LINK_LIBRARIES(async_mysql_test
  ${LtIO_LINKER_LIBS}
  ltbase
)
set_target_properties(async_mysql_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
