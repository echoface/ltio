ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

SET(BASE_SOURCES
  time/timestamp.cc
  time/time_utils.cc
  utils/sys_error.cc
  utils/ns_convertor.cc
  utils/string/str_utils.cc
  #coroutine
  coroutine/coroutine.cc
  coroutine/wait_group.cc
  coroutine/coroutine_runner.cc
  #new eventloop
  message_loop/event.cc
  message_loop/fd_event.cc
  message_loop/event_pump.cc
  message_loop/linux_signal.cpp
  message_loop/file_util_linux.cc
  message_loop/io_multiplexer.cc
  message_loop/io_mux_epoll.cc
  message_loop/timer_task_queue.cc
  message_loop/message_loop.cc
  message_loop/timeout_event.cc
  message_loop/repeating_timer.cc
  #spin lock
  #memory
  memory/spin_lock.cc
  memory/lazy_instance.cc
  # gzip compression utils
  utils/gzip/gzip_utils.cc
  # closure
  closure/location.cc
)

ADD_LIBRARY(ltbase STATIC ${BASE_SOURCES})
ltio_default_properties(ltbase)

if (LTIO_BUILD_SHARED_LIBS)
  ADD_LIBRARY(ltbase_shared SHARED ${BASE_SOURCES})
  ltio_default_properties(ltbase_shared)
  set_target_properties(ltbase_shared PROPERTIES OUTPUT_NAME "ltbase")
  # make shared library and static lib exist same time
  set_target_properties(ltbase PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  set_target_properties(ltbase_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif(LTIO_BUILD_SHARED_LIBS)

ADD_EXECUTABLE(eventloop_test
  message_loop/test/run_loop_test.cc
)
TARGET_LINK_LIBRARIES(eventloop_test
  PUBLIC ltbase
  PUBLIC 3rd
  PUBLIC coro
  PUBLIC ${GLOG_LIBRARIES}
  ${LtIO_LINKER_LIBS}
  PUBLIC ${GFLAGS_LIBRARIES}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

ADD_EXECUTABLE(eventloop_benchmark
  message_loop/test/benchmark.cc
)
TARGET_LINK_LIBRARIES(eventloop_benchmark
  PUBLIC ltbase
  PUBLIC 3rd
  PUBLIC coro
  PUBLIC ${GLOG_LIBRARIES}
  ${LtIO_LINKER_LIBS}
  PUBLIC ${GFLAGS_LIBRARIES}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)
