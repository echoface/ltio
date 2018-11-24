SET(BASE_SOURCES
  time/timestamp.cc
  time/time_utils.cc
  utils/ns_convertor.cc
  #coroutine
  coroutine/coroutine.cc
  coroutine/coroutine_runner.cc
  #new eventloop
  message_loop/fd_event.cc
  message_loop/timer_event.cc
  message_loop/event_pump.cc
  message_loop/linux_signal.cpp
  message_loop/file_util_linux.cc
  message_loop/io_multiplexer.cc
  message_loop/io_multiplexer_epoll.cc
  message_loop/timer_task_queue.cc
  message_loop/message_loop.cc
  message_loop/timeout_event.cc
  #spin lock
  spin_lock.cc
  #memory
  memory/lazy_instance.cc
  # gzip compression utils
  utils/gzip/gzip_utils.cc
  # closure
  closure/location.cc
)

SET(LTBASE_LINKLIBS
  3rd
  coro
  gflags
  unwind
  pthread
  timeout
  ${GLOG_LIBRARY}
  ${ZLIB_LIBRARIES}
)

ADD_LIBRARY(ltbase STATIC
  ${BASE_SOURCES}
)
TARGET_LINK_LIBRARIES(ltbase
  ${LTBASE_LINKLIBS}
)

ADD_EXECUTABLE(eventloop_test
  message_loop/test/run_loop_test.cc
)
TARGET_LINK_LIBRARIES(eventloop_test
  ltbase
  ${LTBASE_LINKLIBS}
)

ADD_EXECUTABLE(eventloop_benchmark
  message_loop/test/benchmark.cc
)
TARGET_LINK_LIBRARIES(eventloop_benchmark
  ltbase
  ${LTBASE_LINKLIBS}
)

# build base unittest
ADD_SUBDIRECTORY(unittest)
