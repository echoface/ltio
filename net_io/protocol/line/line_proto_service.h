#ifndef NET_LINE_PROTO_SERVICE_H
#define NET_LINE_PROTO_SERVICE_H

#include "protocol/proto_service.h"

namespace net {

class LineProtoService : public ProtoService {
public:
  LineProtoService();
  ~LineProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataReceived(const RefTcpChannel &, IOBuffer *) override;

  bool CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) override;

  bool SendRequestMessage(const RefProtocolMessage &message) override;
  bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) override;
};

}
#endif
