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

#ifndef _LT_NET_QUEUED_CLIENT_CHANNEL_H
#define _LT_NET_QUEUED_CLIENT_CHANNEL_H

#include <net_io/codec/codec_message.h>
#include <net_io/codec/codec_service.h>
#include <net_io/net_callback.h>
#include <net_io/tcp_channel.h>
#include <list>
#include "client_channel.h"

namespace lt {
namespace net {

class QueuedChannel;
typedef std::shared_ptr<QueuedChannel> RefQueuedChannel;

class QueuedChannel : public ClientChannel, public EnableShared(QueuedChannel) {
public:
  static RefQueuedChannel Create(Delegate*, const RefCodecService&);
  ~QueuedChannel();

  void StartClientChannel() override;
  void SendRequest(RefCodecMessage request) override;

private:
  QueuedChannel(Delegate*, const RefCodecService&);

  bool TrySendNext();
  void OnRequestTimeout(WeakCodecMessage request);

  // override form ProtocolServiceDelegate
  void BeforeCloseChannel() override;
  void OnCodecMessage(const RefCodecMessage& res) override;
  void OnCodecClosed(const RefCodecService& service) override;

private:
  RefCodecMessage ing_request_;
  std::list<RefCodecMessage> waiting_list_;
};

}  // namespace net
}  // namespace lt
#endif
