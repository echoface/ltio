#ifndef NET_LINE_PROTO_SERVICE_H
#define NET_LINE_PROTO_SERVICE_H

#include "base/message_loop/message_loop.h"
#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

class LineCodecService : public CodecService {
public:
  LineCodecService(base::MessageLoop* loop);
  ~LineCodecService();

  // override from CodecService
  void OnStatusChanged(const SocketChannel*) override;
  void OnDataFinishSend(const SocketChannel*) override;
  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

  bool EncodeToChannel(CodecMessage* message) override;
  bool EncodeResponseToChannel(const CodecMessage* req, CodecMessage* res) override;
};

}}
#endif
