
ADD_EXECUTABLE(acceptor_test
  net_io/acceptor_test.cc
)
TARGET_LINK_LIBRARIES(acceptor_test
  PUBLIC ltio
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

ADD_EXECUTABLE(sample_ltserver
  net_io/sample_ltserver.cc
)
TARGET_LINK_LIBRARIES(sample_ltserver
  PUBLIC ltio
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

ADD_EXECUTABLE(fw_rapid_sample_server
  net_io/fw_rapid_server.cc
)
TARGET_LINK_LIBRARIES(fw_rapid_sample_server
  PUBLIC ltio
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

ADD_EXECUTABLE(io_service_test
  net_io/io_service_test.cc
)
TARGET_LINK_LIBRARIES(io_service_test
  PUBLIC ltio
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

ADD_EXECUTABLE(lt_client_cli
  net_io/lt_client_cli.cc
)
TARGET_LINK_LIBRARIES(lt_client_cli
  PUBLIC ltio
  PUBLIC lt3rd
  PUBLIC coro
  ${LtIO_LINKER_LIBS}
  PUBLIC ${LIBUNWIND_LIBRARIES}
)

