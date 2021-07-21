#ifndef LTIO_NET_WEBSOCKET_MESSAGE_H_H
#define LTIO_NET_WEBSOCKET_MESSAGE_H_H

#include "net_io/codec/codec_message.h"
#include "websocket_parser.h"

namespace lt {
namespace net {

class WebsocketMessage : public CodecMessage {
public:
  ~WebsocketMessage();

  WebsocketMessage(MessageType type);

  WebsocketMessage(MessageType type, int opcode);

  const std::string& Payload() const {
    return payload_;
  }

  void AppendData(const char* data, size_t len);

  int OpCode() const {return opcode_;}

  void SetOpCode(int opcode) {opcode_ = opcode;}

  bool IsPing() const {
    return opcode_ == WS_OP_PING;
  }
  bool IsPong() const {
    return opcode_ == WS_OP_PONG;
  }
  bool IsClose() const {
    return opcode_ == WS_OP_CLOSE;
  }
  bool IsText() const {
    return opcode_ == WS_OP_TEXT;
  }
  bool IsBinary() const {
    return opcode_ == WS_OP_BINARY;
  }

  const std::string Dump() const override;
private:
  //friend class WSCodecService;

  int opcode_;

  std::string payload_;
};

using RefWebsockMessage = std::shared_ptr<WebsocketMessage>;

}
}  // namespace lt

#endif
