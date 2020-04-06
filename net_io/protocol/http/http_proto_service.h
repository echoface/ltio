#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include <vector>
#include <string>
#include "base/message_loop/message_loop.h"
#include "parser_context.h"

#include "net_io/net_callback.h"
#include "net_io/protocol/proto_service.h"
#include "thirdparty/http_parser/http_parser.h"

namespace lt {
namespace net {

class HttpProtoService;
typedef std::shared_ptr<HttpProtoService> RefHttpProtoService;

class HttpProtoService : public ProtoService {
public:
  HttpProtoService(base::MessageLoop* loop);
  ~HttpProtoService();

  // override from ProtoService
  void OnStatusChanged(const SocketChannel*) override;
  void OnDataFinishSend(const SocketChannel*) override;
  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

  static bool RequestToBuffer(const HttpRequest*, IOBuffer*);
  static bool ResponseToBuffer(const HttpResponse*, IOBuffer*);

  void BeforeSendRequest(HttpRequest*);
  bool EncodeToChannel(ProtocolMessage* message) override;

  bool BeforeSendResponseMessage(const HttpRequest*, HttpResponse*);
  bool EncodeResponseToChannel(const ProtocolMessage* req, ProtocolMessage* res) override;

  const RefProtocolMessage NewResponse(const ProtocolMessage*) override;
private:
  bool ParseHttpRequest(const RefTcpChannel&, IOBuffer*);
  bool ParseHttpResponse(const RefTcpChannel&, IOBuffer*);

  ReqParseContext* request_context_;
  ResParseContext* response_context_;
  static http_parser_settings req_parser_settings_;
  static http_parser_settings res_parser_settings_;
};

}}
#endif
