

ADD_EXECUTABLE(log_metrics_test
  component/log_metrics_test.cc
)

TARGET_LINK_LIBRARIES(log_metrics_test
  PUBLIC ltio
  ${LINK_DEP_LIBS}
  ${LtIO_LINKER_LIBS}
  ${LIBUNWIND_LIBRARIES}
)

if (NOT LTIO_DISABLE_ASYNCMYSQL)
  ADD_EXECUTABLE(asyncmysql_test
    component/asyncmysql_test.cc
    )
  TARGET_LINK_LIBRARIES(asyncmysql_test
    PUBLIC ltasyncmysql
    ${LINK_DEP_LIBS}
    ${LtIO_LINKER_LIBS}
    ${LIBUNWIND_LIBRARIES}
    )
endif()
