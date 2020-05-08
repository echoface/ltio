ADD_DEFINITIONS(-std=c++11)

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

  #protocol codec
  codec/codec_message.cc
  codec/codec_service.cc
  codec/codec_factory.cc

  codec/line/line_message.cc
  codec/line/line_codec_service.cc

  codec/http/http_request.cc
  codec/http/http_response.cc
  codec/http/http_constants.cc
  codec/http/parser_context.cc
  codec/http/http_codec_service.cc

  codec/raw/raw_message.cc

  codec/redis/redis_request.cc
  codec/redis/redis_response.cc
  codec/redis/resp_codec_service.cc
  #dispatcher
  dispatcher/coro_dispatcher.cc
  dispatcher/workload_dispatcher.cc

  #server
  server/raw_server/raw_server.cc
  server/http_server/http_server.cc
  server/http_server/http_context.cc

  #clients source
  clients/client.cc
  clients/client_channel.cc
  clients/async_channel.cc
  clients/queued_channel.cc
  clients/client_connector.cc

  # client rounter
  clients/router/maglev.cc
  clients/router/hash_router.h
  clients/router/maglev_router.cc
  clients/router/ringhash_router.cc
  clients/router/roundrobin_router.cc
)

add_library(ltnet_objs OBJECT ${LTNET_SOURCE})
ltio_default_properties(ltnet_objs)
target_link_libraries(ltnet_objs
  PRIVATE fmt::fmt
  )
