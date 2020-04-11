#ifndef _LT_NET_QUEUED_CLIENT_CHANNEL_H 
#define _LT_NET_QUEUED_CLIENT_CHANNEL_H

#include <list>
#include "client_channel.h"
#include <net_io/tcp_channel.h>
#include <net_io/net_callback.h>
#include <net_io/codec/codec_message.h>
#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

class QueuedChannel;
typedef std::shared_ptr<QueuedChannel> RefQueuedChannel;

class QueuedChannel : public ClientChannel,
                      public EnableShared(QueuedChannel) {
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
  void OnProtocolServiceGone(const RefCodecService& service) override;
private:
  RefCodecMessage ing_request_;
  std::list<RefCodecMessage> waiting_list_;
};

}}//end namespace
#endif
