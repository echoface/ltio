#ifndef _LT_NET_QUEUED_CLIENT_CHANNEL_H 
#define _LT_NET_QUEUED_CLIENT_CHANNEL_H

#include <list>
#include "tcp_channel.h"
#include "net_callback.h"
#include "client_channel.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service.h"

namespace net {

class QueuedChannel;
typedef std::shared_ptr<QueuedChannel> RefQueuedChannel;

class QueuedChannel : public ClientChannel,
                      public std::enable_shared_from_this<QueuedChannel> {
public:
	static RefQueuedChannel Create(Delegate*, RefProtoService&);
  ~QueuedChannel();

  void StartClient() override;
  void SendRequest(RefProtocolMessage request) override;
private:
	QueuedChannel(Delegate*, RefProtoService&);

  bool TrySendNext();
  void OnRequestTimeout(WeakProtocolMessage request);

  void OnResponseMessage(const RefProtocolMessage& res);
	void OnProtocolServiceGone(const RefProtoService& service) override;
private:
  RefProtocolMessage ing_request_;
  std::list<RefProtocolMessage> waiting_list_;
};

}//end namespace
#endif
