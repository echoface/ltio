

ADD_EXECUTABLE(log_metrics_test
  component/log_metrics_test.cc
)

TARGET_LINK_LIBRARIES(log_metrics_test
  PUBLIC ltio
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

if (NOT LTIO_DISABLE_ASYNCMYSQL)
  ADD_EXECUTABLE(asyncmysql_test
    component/asyncmysql_test.cc
    )
  TARGET_LINK_LIBRARIES(asyncmysql_test
    PUBLIC ltasyncmysql
    PUBLIC ltbase
    PUBLIC lt3rd
    PUBLIC coro
    ${LtIO_LINKER_LIBS}
    PUBLIC ${LIBUNWIND_LIBRARIES}
    )
endif()
