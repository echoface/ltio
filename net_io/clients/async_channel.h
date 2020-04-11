#ifndef _LT_NET_ASYNC_CLIENT_CHANNEL_H
#define _LT_NET_ASYNC_CLIENT_CHANNEL_H

#include <list>
#include <unordered_map>
#include <net_io/tcp_channel.h>
#include <net_io/net_callback.h>
#include <net_io/codec/codec_message.h>
#include <net_io/codec/codec_service.h>

#include "client_channel.h"

namespace lt {
namespace net {

class AsyncChannel;

REF_TYPEDEFINE(AsyncChannel);

class AsyncChannel : public ClientChannel,
                     public EnableShared(AsyncChannel) {
public:
	static RefAsyncChannel Create(Delegate*, const RefCodecService&);
  ~AsyncChannel();

  void StartClientChannel() override;
  void SendRequest(RefCodecMessage request) override;
private:
	AsyncChannel(Delegate*, const RefCodecService&);
  void OnRequestTimeout(WeakCodecMessage request);

  //override protocolServiceDelegate
  void BeforeCloseChannel() override;
  void OnCodecMessage(const RefCodecMessage& res) override;
	void OnProtocolServiceGone(const RefCodecService& service) override;
private:
  std::unordered_map<uint64_t, RefCodecMessage> in_progress_;
};

}}//end namespace
#endif
