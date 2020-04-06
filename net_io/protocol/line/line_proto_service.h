#ifndef NET_LINE_PROTO_SERVICE_H
#define NET_LINE_PROTO_SERVICE_H

#include "base/message_loop/message_loop.h"
#include <net_io/protocol/proto_service.h>

namespace lt {
namespace net {

class LineProtoService : public ProtoService {
public:
  LineProtoService(base::MessageLoop* loop);
  ~LineProtoService();

  // override from ProtoService
  void OnStatusChanged(const SocketChannel*) override;
  void OnDataFinishSend(const SocketChannel*) override;
  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

  bool EncodeToChannel(ProtocolMessage* message) override;
  bool EncodeResponseToChannel(const ProtocolMessage* req, ProtocolMessage* res) override;
};

}}
#endif
