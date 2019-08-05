cmake_minimum_required(VERSION 2.8)

ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

aux_source_directory(. LOGMETRICS_SOURCES)

ADD_LIBRARY(ltmetrics STATIC ${LOGMETRICS_SOURCES})

ADD_EXECUTABLE(log_metrics_test
  ./test/log_metrics_test.cc
)

TARGET_LINK_LIBRARIES(log_metrics_test
  logmetrics
	${GLOG_LIBRARY}
  pthread
)
set_target_properties(log_metrics_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
