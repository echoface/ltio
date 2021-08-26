#ifndef LTIO_NET_WEBSOCKET_MESSAGE_H_H
#define LTIO_NET_WEBSOCKET_MESSAGE_H_H

#include "net_io/codec/codec_message.h"
#include "websocket_parser.h"

namespace lt {
namespace net {

class WebsocketFrame : public CodecMessage {
public:
  static WebsocketFrame* NewCloseFrame(uint16_t code);

  ~WebsocketFrame();

  WebsocketFrame();

  WebsocketFrame(int opcode);

  const std::string_view Payload() const {
    return std::string_view((char*)payload_.data(), payload_.size());
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

  std::vector<uint8_t> payload_;
};

using RefWebsocketFrame = std::shared_ptr<WebsocketFrame>;

}
}  // namespace lt

#endif
