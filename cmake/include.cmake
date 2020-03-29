# top level include dir

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_BINARY_DIR}/thirdparty

  ${GLOG_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}/mysql
  ${MYSQL_INCLUDE_DIR}/mariadb
)
