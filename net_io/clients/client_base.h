/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

}  // namespace net
}  // namespace lt
#endif
