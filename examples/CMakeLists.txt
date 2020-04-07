
ADD_EXECUTABLE(eventloop_test
  base/run_loop_test.cc
)
TARGET_LINK_LIBRARIES(eventloop_test
  PUBLIC ltbase
  PUBLIC lt3rd
  PUBLIC coro
  PUBLIC ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

ADD_EXECUTABLE(loop_benchmark
  base/loop_benchmark.cc
)
TARGET_LINK_LIBRARIES(loop_benchmark
  PUBLIC ltbase
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)
