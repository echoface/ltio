add_library(ltasyncmysql STATIC
  query_session.cc
  mysql_async_con.cc
  mysql_client_impl.cc
  )

ltio_default_properties(ltasyncmysql)
target_include_directories(ltasyncmysql
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
  )
