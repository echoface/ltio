ADD_EXECUTABLE(acceptor_test
  net_io/acceptor_test.cc
)
TARGET_LINK_LIBRARIES(acceptor_test
  ltio
)

ADD_EXECUTABLE(simple_ltserver
  net_io/simple_ltserver.cc
)
TARGET_LINK_LIBRARIES(simple_ltserver
  ltio
)

ADD_EXECUTABLE(fw_rapid_simple_server
  net_io/fw_rapid_server.cc
)
TARGET_LINK_LIBRARIES(fw_rapid_simple_server
  ltio
)

ADD_EXECUTABLE(io_service_test
  net_io/io_service_test.cc
)
TARGET_LINK_LIBRARIES(io_service_test
  ltio
)

ADD_EXECUTABLE(lt_client_cli
  net_io/lt_client_cli.cc
)
TARGET_LINK_LIBRARIES(lt_client_cli
  ltio
)
