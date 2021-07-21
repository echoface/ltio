
#include "ws_message.h"
#include "fmt/core.h"

namespace lt {
namespace net {

WebsocketMessage::~WebsocketMessage() {}

WebsocketMessage::WebsocketMessage(MessageType type) : CodecMessage(type) {}

WebsocketMessage::WebsocketMessage(MessageType type, int opcode)
  : CodecMessage(type),
    opcode_(opcode) {}

void WebsocketMessage::AppendData(const char* data, size_t len) {
  payload_.append(data, len);
}

const std::string WebsocketMessage::Dump() const {
  return fmt::format("opcode:{}, payload:{}", opcode_, payload_);
}

}  // namespace net
}  // namespace lt
