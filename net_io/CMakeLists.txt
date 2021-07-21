
set(LTNET_SOURCE
  channel.cc
  io_buffer.cc
  io_service.cc
  tcp_channel.cc
  socket_utils.cc
  socket_acceptor.cc
  url_utils.cc

  #base/address.cc
  base/ip_address.cc
  base/ip_endpoint.cc
  base/address_family.cc
  base/sockaddr_storage.cc

  #base/load_balance
  base/load_balance/maglev2.cc

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
  #dispatcher
  dispatcher/coro_dispatcher.cc
  dispatcher/workload_dispatcher.cc

  #server
  server/generic_server.h
  server/ws_server/ws_server.cc
  server/raw_server/raw_server.cc
  server/http_server/http_context.cc

  #clients source
  clients/client.cc
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
)

if (LTIO_WITH_OPENSSL)
  list(APPEND LTNET_SOURCE tcp_channel_ssl.cc)
endif()

add_library(ltnet_objs OBJECT ${LTNET_SOURCE})
ltio_default_properties(ltnet_objs)
target_include_directories(ltnet_objs
    PUBLIC
    $<TARGET_PROPERTY:fmt,INTERFACE_INCLUDE_DIRECTORIES>
    )
