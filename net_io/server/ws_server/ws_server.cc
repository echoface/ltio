#include "ws_server.h"

namespace lt {
namespace net {

WsServer::WsServer() {

}

WsServer::~WsServer() {

}

void WsServer::Register(const std::string& topic, WSService* impl) {
  wss_[topic] = impl;
}


}  // namespace net
}  // namespace lt
