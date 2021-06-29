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

#ifndef LT_NET_CLIENT_ROUTER_H_H
#define LT_NET_CLIENT_ROUTER_H_H

#include <net_io/clients/client.h>
#include <net_io/codec/codec_message.h>
#include <atomic>
#include <memory>
#include <vector>

namespace lt {
namespace net {

REF_TYPEDEFINE(Client);
/**
 * Router:
 * ClientRouter manager a group of client(a client has N connection)
 *
 * route reqeust to diffrent client do network request
 * */
class ClientRouter {
public:
  ClientRouter(){};
  virtual ~ClientRouter(){};

  //** some of router need re-calculate values and adjustment
  //** after StartRouter, DO NOT AddClient ANY MORE
  virtual void StartRouter(){};

  virtual void AddClient(RefClient&& client) = 0;

  virtual RefClient GetNextClient(const std::string& hash_key,
                                  CodecMessage* hint_message = NULL) = 0;
};

}  // namespace net
}  // namespace lt
#endif
