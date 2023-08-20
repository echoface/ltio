
set(LTNET_SOURCE
  channel.cc
  io_buffer.cc
  io_service.cc
  tcp_channel.cc
  socket_utils.cc
  socket_acceptor.cc
  url_utils.cc

  #base/address.cc
  common/ip_address.cc
  common/ip_endpoint.cc
  common/address_family.cc
  common/sockaddr_storage.cc
  #base/load_balance
  common/load_balance/maglev2.cc

  #protocol codec
  codec/codec_message.cc
  codec/codec_service.cc
  codec/codec_factory.cc

  codec/line/line_message.cc
  codec/line/line_codec_service.cc

  codec/http/http_message.cc
  codec/http/http_constants.cc
  codec/http/http_codec_service.cc

  codec/websocket/ws_util.cc
  codec/websocket/ws_message.cc
  codec/websocket/websocket_parser.c
  codec/websocket/ws_codec_service.cc

  codec/raw/raw_message.cc

  codec/redis/redis_request.cc
  codec/redis/redis_response.cc
  codec/redis/resp_codec_service.cc

  #server
  server/generic_server.h
  server/ws_server/ws_server.cc
  server/raw_server/raw_server.cc
  server/http_server/http_context.cc

  #clients source
  clients/client.cc
  clients/ws_client.cc
  clients/client_channel.cc
  clients/async_channel.cc
  clients/queued_channel.cc
  clients/client_connector.cc

  # client rounter
  clients/router/hash_router.h
  clients/router/maglev_router.cc
  clients/router/ringhash_router.cc
  clients/router/roundrobin_router.cc

  #udp_io
  udp_io/udp_context.cc
  udp_io/udp_service.cc

  #coro io service
  coronet/io_service.cc
)

if (LTIO_WITH_OPENSSL)
  list(APPEND LTNET_SOURCE tcp_channel_ssl.cc)
endif()

if (LTIO_WITH_HTTP2)
  list(APPEND LTNET_SOURCE codec/http/h2/nghttp2_util.cc)
  list(APPEND LTNET_SOURCE codec/http/h2/h2_codec_service.cc)
endif()

if (LTIO_WITH_HTTP2 AND LTIO_WITH_OPENSSL)
  list(APPEND LTNET_SOURCE codec/http/h2/h2_tls_context.cc)
endif()

target_sources(ltio PRIVATE ${LTNET_SOURCE})
target_sources(ltio_shared PRIVATE ${LTNET_SOURCE})

add_executable(ut_ltnetio
    common/common_unittest.cc
    common/load_balance/maglev_unittest.cc
    clients/client_unittest.cc
    clients/router/router_unittest.cc
    )

target_link_libraries(ut_ltnetio
    PRIVATE
    ltio
    profiler
    Catch2::Catch2WithMain
    )
