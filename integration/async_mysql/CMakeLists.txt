ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

aux_source_directory(. ASYNC_MYSQL_SOURCES)

ADD_LIBRARY(ltmysql STATIC ${ASYNC_MYSQL_SOURCES})

ADD_EXECUTABLE(asyc_mysql_test
  ./test/async_main.cc
)

TARGET_LINK_LIBRARIES(asyc_mysql_test
  ltmysql
  ${MYSQL_LIBRARIES}
  ${LTBASE_LINKLIBS}
	${GLOG_LIBRARY}
  pthread
)
set_target_properties(asyc_mysql_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
