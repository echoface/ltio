#ifndef LT_NET_CLIENT_BASE_H_H
#define LT_NET_CLIENT_BASE_H_H

#include <cstdint>
namespace lt {
namespace net {

typedef struct ClientConfig {
  uint32_t connections = 1;
  // 0: without heartbeat needed
  // heartbeat need protocol service supported
  uint32_t heartbeat_ms = 0;

  uint32_t recon_interval = 5000;

  uint16_t connect_timeout = 5000;

  uint32_t message_timeout = 5000;
} ClientConfig;

}}//end lt::net
#endif

