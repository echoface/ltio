#ifndef _LT_NET_WS_CODEC_SERVICE_H_
#define _LT_NET_WS_CODEC_SERVICE_H_

#include "ws_message.h"

#include "net_io/codec/codec_service.h"
#include "net_io/codec/http/parser_context.h"
#include "net_io/codec/websocket/websocket_parser.h"

namespace lt {
namespace net {

// public api for message interactive based on bi-stream connection
class Websocket : public base::LinkedList<Websocket>::Node {
public:
  Websocket() {}

  virtual bool Send(RefWebsocketFrame message) { return false; };
};

using WSCallback = std::function<void(Websocket* ws)>;
using WSMsgCallback =
    std::function<void(Websocket* ws, const RefWebsocketFrame&)>;

using WebsocketParser = struct websocket_parser;

class WSCodecService : public CodecService, public Websocket {
public:
  WSCodecService(base::MessageLoop* loop);

  virtual ~WSCodecService();

  void StartProtocolService() override;

  void BeforeCloseService() override{};

  void AfterChannelClosed() override{};

  /* feature indentify*/
  // async clients request
  bool KeepSequence() override { return false; };

  bool KeepHeartBeat() override { return false; }

  bool Send(RefWebsocketFrame message) override;

  bool SendRequest(CodecMessage* message) override;

  // for bi-stream service/client, req may nil,
  // the diff of SendResponse/SendRequest just identify sender
  bool SendResponse(const CodecMessage* req, CodecMessage* res) override;

  const RefCodecMessage NewHeartbeat() override { return NULL; }

  const RefCodecMessage NewResponse(const CodecMessage*) override {
    return NULL;
  }

  bool UseSSLChannel() const override { return protocol_ == "wss"; };

  void CommitHttpRequest(const RefHttpRequest&& request);

  void CommitHttpResponse(const RefHttpResponse&& response);

  const std::string& TopicPath() const { return ws_path_; }

  // client api: format /xxx/path?queries
  void SetTopicPath(const std::string& path) { ws_path_ = path;}

protected:
  using HttpReqParser = HttpParser<WSCodecService, HttpRequest>;
  using HttpResParser = HttpParser<WSCodecService, HttpResponse>;

  static int OnFrameBegin(websocket_parser* parser);

  static int OnFrameBody(websocket_parser* parser,
                         const char* data,
                         size_t len);

  static int OnFrameFinish(websocket_parser* parser);

  void OnHandshakeReqData(IOBuffer* buf);

  void OnHandshakeResData(IOBuffer* buf);

  void CommitFrameMessage();

  void OnDataReceived(IOBuffer*) override;

  void init_http_parser();

  void finalize_http_parser();

  HttpReqParser* http_req_parser() { return http_parser_.req_parser; }
  HttpResParser* http_res_parser() { return http_parser_.res_parser; }

private:
  enum handshake_state {
    HS_None = 0,
    HS_SUCCEESS = 1,
    HS_WRONGMSG = 2,
  };
  union HandleShakeParser {
    HttpReqParser* req_parser;
    HttpResParser* res_parser;
  };
  HandleShakeParser http_parser_;

  handshake_state hs_state = HS_None;

  websocket_parser parser_;

  websocket_parser_settings settings_;

  /*
   * The "Request-URI" of the GET method [RFC2616] is used
   * to identify the endpoint of the WebSocket connection,
   * both to allow multiple domains to be served from one IP address
   */
  std::string ws_path_;

  std::string sec_key_;

  std::string sec_accept_;

  RefWebsocketFrame current_;

  DISALLOW_COPY_AND_ASSIGN(WSCodecService);
};

}  // namespace net
}  // namespace lt
#endif
