SET(LINK_DEP_LIBS
  PUBLIC ltio
  )

ADD_EXECUTABLE(eventloop_test
  base/run_loop_test.cc
  )
TARGET_LINK_LIBRARIES(eventloop_test
  ${LINK_DEP_LIBS}
  )


ADD_EXECUTABLE(loop_benchmark
  base/loop_benchmark.cc
  )
TARGET_LINK_LIBRARIES(loop_benchmark
  ${LINK_DEP_LIBS}
  )

include(net_io/build_targets.cmake)

include(component/build_targets.cmake)
