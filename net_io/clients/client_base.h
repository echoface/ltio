#ifndef LT_NET_CLIENT_BASE_H_H
#define LT_NET_CLIENT_BASE_H_H

namespace lt {
namespace net {

typedef struct {
  uint32_t connections = 1;
  // heart beat_ms only work for protocol service supported
  // 0 mean not need heart beat keep
  uint32_t heart_beat_ms = 0;

  uint32_t recon_interval = 5000;

  uint16_t connect_timeout = 5000;

  uint32_t message_timeout = 5000;
} ClientConfig;

}}//end lt::net
#endif

