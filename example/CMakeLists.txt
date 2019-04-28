ADD_DEFINITIONS(-std=c++11)

#ADD_EXECUTABLE(coro_task_test
  #coro_schedule_test.cc
  #)

  #add_dependencies(coro_task_test
  #coro
  #glog
  #base_shared
  #)
  #TARGET_LINK_LIBRARIES(coro_task_test
  #libcoro.a
  #libglog.a
  #-levent
  #-lbase_shared
  #-lpthread
  #)
ADD_EXECUTABLE(mysql_lt_client
  mysql_integration/async_main.cc
  mysql_integration/query_session.cc
  mysql_integration/mysql_async_con.cc
)
TARGET_LINK_LIBRARIES(mysql_lt_client
  ltbase
  ${MYSQL_LIBRARIES}
  ${LTBASE_LINKLIBS}
)

#ADD_EXECUTABLE(v8_node
#  v8_shell.cc
#)
#TARGET_LINK_LIBRARIES(v8_node
#  ltbase
#  #mariadbclient
#  ${LTBASE_LINKLIBS}
#)
