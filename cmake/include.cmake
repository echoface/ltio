# top level include dir

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/base

  #net
  ${PROJECT_SOURCE_DIR}/net_io

  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_BINARY_DIR}/thirdparty

  ${PROJECT_SOURCE_DIR}/components

  #coro
  ${PROJECT_BINARY_DIR}/thirdparty/libcoro

  ${GLOG_INCLUDE_DIR}

  ${MYSQL_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}/mysql
  ${MYSQL_INCLUDE_DIR}/mariadb
)
