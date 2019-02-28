#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include <vector>
#include <string>
#include "net_callback.h"
#include "parser_context.h"
#include "protocol/proto_service.h"
#include "http_parser/http_parser.h"

namespace net {
class HttpProtoService;
typedef std::shared_ptr<HttpProtoService> RefHttpProtoService;

class HttpProtoService : public ProtoService {
public:
  HttpProtoService();
  ~HttpProtoService();

  // override from ProtoService
  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;
  void OnDataReceived(const RefTcpChannel &, IOBuffer *) override;

  static bool RequestToBuffer(const HttpRequest*, IOBuffer*);
  static bool ResponseToBuffer(const HttpResponse*, IOBuffer*);

  void BeforeSendRequest(HttpRequest*);
  bool SendRequestMessage(const RefProtocolMessage &message) override;

  bool BeforeSendResponseMessage(HttpRequest*, HttpResponse*);
  bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) override;

  const RefProtocolMessage NewResponseFromRequest(const RefProtocolMessage &) override;
private:
  bool ParseHttpRequest(const RefTcpChannel&, IOBuffer*);
  bool ParseHttpResponse(const RefTcpChannel&, IOBuffer*);

  ReqParseContext* request_context_;
  ResParseContext* response_context_;
  static http_parser_settings req_parser_settings_;
  static http_parser_settings res_parser_settings_;
};

}
#endif
