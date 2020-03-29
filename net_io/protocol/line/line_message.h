#ifndef _NET_LINE_PROTO_MESSAGE_H
#define _NET_LINE_PROTO_MESSAGE_H

#include <net_io/protocol/proto_service.h>
#include <net_io/protocol/proto_message.h>

namespace lt {
namespace net {

class LineMessage : public ProtocolMessage {
public:
  typedef LineMessage ResponseType;
  LineMessage(MessageType t);
  ~LineMessage();
  std::string& MutableBody();
  const std::string& Body() const;
private:
  std::string body_;
};

}}
#endif
