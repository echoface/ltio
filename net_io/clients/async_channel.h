#ifndef _LT_NET_ASYNC_CLIENT_CHANNEL_H
#define _LT_NET_ASYNC_CLIENT_CHANNEL_H

#include <list>
#include <unordered_map>
#include <net_io/tcp_channel.h>
#include <net_io/net_callback.h>
#include <net_io/protocol/proto_message.h>
#include <net_io/protocol/proto_service.h>

#include "client_channel.h"

namespace lt {
namespace net {

class AsyncChannel;

typedef std::shared_ptr<AsyncChannel> RefAsyncChannel;

class AsyncChannel : public ClientChannel,
                     public EnableShared(AsyncChannel) {
public:
	static RefAsyncChannel Create(Delegate*, const RefProtoService&);
  ~AsyncChannel();

  void StartClient() override;
  void SendRequest(RefProtocolMessage request) override;
private:
	AsyncChannel(Delegate*, const RefProtoService&);
  void OnRequestTimeout(WeakProtocolMessage request);

  //override protocolServiceDelegate
  void BeforeCloseChannel() override;
  void OnProtocolMessage(const RefProtocolMessage& res) override;
	void OnProtocolServiceGone(const RefProtoService& service) override;
private:
  std::unordered_map<uint64_t, RefProtocolMessage> in_progress_;
};

}}//end namespace
#endif
