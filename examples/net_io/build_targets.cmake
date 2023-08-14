ADD_EXECUTABLE(acceptor_test
  net_io/acceptor_test.cc
)
target_link_libraries(acceptor_test
    ltio
    gflags::gflags
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

ADD_EXECUTABLE(http_benchmark_server
  net_io/http_benchmark.cc
)
TARGET_LINK_LIBRARIES(http_benchmark_server
  ltio
)

ADD_EXECUTABLE(lt_http_client
  net_io/lt_http_client.cc
)
TARGET_LINK_LIBRARIES(lt_http_client
  ltio
)

ADD_EXECUTABLE(lt_ws_echo_server
  net_io/ws_echo_server.cc
)
TARGET_LINK_LIBRARIES(lt_ws_echo_server
  ltio
)

ADD_EXECUTABLE(lt_ws_client
  net_io/lt_ws_client.cc
)
TARGET_LINK_LIBRARIES(lt_ws_client
  ltio
)

ADD_EXECUTABLE(lt_coso_io
  net_io/co_so_service.cc
)
TARGET_LINK_LIBRARIES(lt_coso_io
  ltio
)

#if (LTIO_WITH_HTTP2)
ADD_EXECUTABLE(lt_http2_server
  net_io/http2_server.cc
)
TARGET_LINK_LIBRARIES(lt_http2_server
  ltio
)
#endif()
