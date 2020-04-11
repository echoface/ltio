
#include "line_message.h"

namespace lt {
namespace net {

LineMessage::LineMessage(MessageType t)
  : CodecMessage(t) {
}

LineMessage::~LineMessage() {

}
std::string& LineMessage::MutableBody() {
  return body_;
}
const std::string& LineMessage::Body() const {
  return body_;
}

}};//end namespace net
