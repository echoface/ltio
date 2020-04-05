#ifndef _LT_NET_QUEUED_CLIENT_CHANNEL_H 
#define _LT_NET_QUEUED_CLIENT_CHANNEL_H

#include <list>
#include "client_channel.h"
#include <net_io/tcp_channel.h>
#include <net_io/net_callback.h>
#include <net_io/protocol/proto_message.h>
#include <net_io/protocol/proto_service.h>

namespace lt {
namespace net {

class QueuedChannel;
typedef std::shared_ptr<QueuedChannel> RefQueuedChannel;

class QueuedChannel : public ClientChannel,
                      public EnableShared(QueuedChannel) {
public:
	static RefQueuedChannel Create(Delegate*, const RefProtoService&);
  ~QueuedChannel();

  void StartClientChannel() override;
  void SendRequest(RefProtocolMessage request) override;
private:
	QueuedChannel(Delegate*, const RefProtoService&);

  bool TrySendNext();
  void OnRequestTimeout(WeakProtocolMessage request);

  // override form ProtocolServiceDelegate
  void BeforeCloseChannel() override;
  void OnProtocolMessage(const RefProtocolMessage& res) override;
  void OnProtocolServiceGone(const RefProtoService& service) override;
private:
  RefProtocolMessage ing_request_;
  std::list<RefProtocolMessage> waiting_list_;
};

}}//end namespace
#endif
