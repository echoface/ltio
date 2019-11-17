#ifndef NET_LINE_PROTO_SERVICE_H
#define NET_LINE_PROTO_SERVICE_H

#include "protocol/proto_service.h"

namespace lt {
namespace net {

class LineProtoService : public ProtoService {
public:
  LineProtoService();
  ~LineProtoService();

  // override from ProtoService
  void OnStatusChanged(const SocketChannel*) override;
  void OnDataFinishSend(const SocketChannel*) override;
  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

  bool SendRequestMessage(ProtocolMessage* message) override;
  bool SendResponseMessage(const ProtocolMessage* req, ProtocolMessage* res) override;
};

}}
#endif
