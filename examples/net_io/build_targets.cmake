
ADD_EXECUTABLE(acceptor_test
  net_io/acceptor_test.cc
)
TARGET_LINK_LIBRARIES(acceptor_test
  ${LINK_DEP_LIBS}
)

ADD_EXECUTABLE(sample_ltserver
  net_io/sample_ltserver.cc
)
TARGET_LINK_LIBRARIES(sample_ltserver
  ${LINK_DEP_LIBS}
)

ADD_EXECUTABLE(fw_rapid_sample_server
  net_io/fw_rapid_server.cc
)
TARGET_LINK_LIBRARIES(fw_rapid_sample_server
  ${LINK_DEP_LIBS}
)

ADD_EXECUTABLE(io_service_test
  net_io/io_service_test.cc
)
TARGET_LINK_LIBRARIES(io_service_test
  ${LINK_DEP_LIBS}
)

ADD_EXECUTABLE(lt_client_cli
  net_io/lt_client_cli.cc
)
TARGET_LINK_LIBRARIES(lt_client_cli
  ${LINK_DEP_LIBS}
)

