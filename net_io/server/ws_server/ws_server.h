#ifndef _LT_NET_WS_SERVER_H_H
#define _LT_NET_WS_SERVER_H_H

#include "net_io/codec/websocket/ws_codec_service.h"
#include "net_io/io_service.h"
#include "net_io/server/generic_server.h"

namespace lt {
namespace net {

/*
 * usage example:
 *
 * WsServer server;
 * server.WithLoops("");
 * server.WithService("/chat", impl);
 * server.ServeAddress(port);
 *
 * */

class WSService {
  public:
    virtual ~WSService() {};

    virtual void OnOpen(Websocket* ws) = 0;
    virtual void OnClose(Websocket* ws) = 0;
    virtual void OnMessage(Websocket* ws, const RefWebsocketFrame&) = 0;
};

class WsServer : public CodecService::Handler,
                 public BaseServer<DefaultConfigurator> {
public:
  WsServer();

  ~WsServer();

  void Register(const std::string& topic, WSService*);

  void Listen(const std::string& addr) {
    ServeAddress(addr);
  };

  void ServeAddress(const std::string& addr) {
    AsServer()->ServeAddress(addr, AsHandler());
  }

protected:
  void OnConnectionOpen(const RefCodecService& codec) override {

    WSCodecService* s = (WSCodecService*)codec.get();
    VLOG(GLOG_VINFO) << __FUNCTION__ << ", enter, topic:" << s->TopicPath();

    auto iter = wss_.find(s->TopicPath());
    if (iter == wss_.end()) {
      LOG(ERROR) << __FUNCTION__ << " not found service implement";
      return codec->CloseService();
    }

    if (s->GetHandler() != AsHandler()) {
      codec->SetHandler(this);
    }
    iter->second->OnOpen(s);
  }

  void OnConnectionClose(const RefCodecService& codec) override {
    WSCodecService* s = (WSCodecService*)codec.get();

    VLOG(GLOG_VINFO) << __FUNCTION__ << ", enter, topic:" << s->TopicPath();
    auto iter = wss_.find(s->TopicPath());
    if (iter == wss_.end()) {
      LOG(ERROR) << __FUNCTION__ << " not found service implement";
      return;// codec->CloseService();
    }
    iter->second->OnClose(s);
  }

  void OnCodecMessage(const RefCodecMessage& message) override {

    RefCodecService codec = message->GetIOCtx().codec.lock();
    CHECK(codec);

    WSCodecService* s = (WSCodecService*)codec.get();
    VLOG(GLOG_VINFO) << __FUNCTION__ << ", enter, topic:" << s->TopicPath();

    auto iter = wss_.find(s->TopicPath());
    if (iter == wss_.end()) {
      LOG(ERROR) << __FUNCTION__ << " not found service implement";
      return;// codec->CloseService();
    }

    WebsocketFrame* ws_msg = (WebsocketFrame*)message.get();

    VLOG(GLOG_VTRACE) << "ws recieve:" << message->Dump();
    iter->second->OnMessage(s, RefCast(WebsocketFrame, message));
  }

private:
  CodecService::Handler* AsHandler() {
    CodecService::Handler* handler = this;
    return handler;
  }

  BaseServer<DefaultConfigurator>* AsServer() {
    return (BaseServer<DefaultConfigurator>*)this;
  }

  std::unordered_map<std::string, WSService*> wss_;
};

}  // namespace net
}  // namespace lt
#endif
