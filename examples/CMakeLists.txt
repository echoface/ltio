
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
