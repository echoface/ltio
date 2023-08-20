#include "ws_message.h"

#include "fmt/core.h"

#include "base/sys/byteorders.h"

namespace lt {
namespace net {

WebsocketFrame* WebsocketFrame::NewCloseFrame(uint16_t code) {
  WebsocketFrame* frame = new WebsocketFrame(WS_OP_CLOSE);
  uint16_t bytes = base::HostToNet16(code);
  frame->AppendData((const char*)&bytes, sizeof(bytes));
  return frame;
}

WebsocketFrame::~WebsocketFrame() {}

WebsocketFrame::WebsocketFrame() : CodecMessage(), opcode_(WS_OP_TEXT) {}

WebsocketFrame::WebsocketFrame(int opcode) : CodecMessage(), opcode_(opcode) {}

void WebsocketFrame::AppendData(const char* data, size_t len) {
  std::copy(data, data + len, std::back_inserter(payload_));
}

const std::string WebsocketFrame::Dump() const {
  return fmt::format("opcode:{}, payload:{}", opcode_, Payload().to_string());
}

}  // namespace net
}  // namespace lt
