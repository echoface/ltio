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

#ifndef _LT_NET_ASYNC_CLIENT_CHANNEL_H
#define _LT_NET_ASYNC_CLIENT_CHANNEL_H

#include <net_io/codec/codec_message.h>
#include <net_io/codec/codec_service.h>
#include <net_io/net_callback.h>
#include <net_io/tcp_channel.h>
#include <list>
#include <unordered_map>

#include "client_channel.h"

namespace lt {
namespace net {

class AsyncChannel;

REF_TYPEDEFINE(AsyncChannel);

class AsyncChannel : public ClientChannel, public EnableShared(AsyncChannel) {
public:
  static RefAsyncChannel Create(Delegate*, const RefCodecService&);
  ~AsyncChannel();

  void StartClientChannel() override;
  void SendRequest(RefCodecMessage request) override;

private:
  AsyncChannel(Delegate*, const RefCodecService&);
  void OnRequestTimeout(WeakCodecMessage request);

  // override protocolServiceDelegate
  void BeforeCloseChannel() override;
  void OnCodecMessage(const RefCodecMessage& res) override;
  void OnProtocolServiceGone(const RefCodecService& service) override;

private:
  std::unordered_map<uint64_t, RefCodecMessage> in_progress_;
};

}  // namespace net
}  // namespace lt
#endif
