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

#ifndef NET_PROTOCOL_SERVICE_FACTORY_H
#define NET_PROTOCOL_SERVICE_FACTORY_H

#include "codec_service.h"

#include <net_io/net_callback.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

typedef std::function<RefCodecService(base::MessageLoop* loop)>
    ProtoserviceCreator;

class CodecFactory {
public:
  static RefCodecService NewServerService(const std::string& proto,
                                          base::MessageLoop* loop);
  static RefCodecService NewClientService(const std::string& proto,
                                          base::MessageLoop* loop);

  static bool HasCreator(const std::string&);

  // not thread safe,
  // this can cover the default protoservice or add new protocol support
  static void RegisterCreator(const std::string, ProtoserviceCreator);

public:
  CodecFactory();

private:
  void InitInnerDefault();

  RefCodecService CreateProtocolService(const std::string&,
                                        base::MessageLoop*,
                                        bool server_service);

  std::unordered_map<std::string, ProtoserviceCreator> creators_;
};

}  // namespace net
}  // namespace lt
#endif
