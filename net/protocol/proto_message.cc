
#include "proto_message.h"

namespace net {

ProtocolMessage::ProtocolMessage(const std::string protocol, MessageType type)
  : type_(type),
    proto_(protocol),
    fail_info_(kNothing),
    responsed_(false) {
  work_context_.loop = NULL;
}

ProtocolMessage::~ProtocolMessage() {
}

const std::string& ProtocolMessage::Protocol() const {
  return proto_;
}

void ProtocolMessage::SetFailInfo(FailInfo reason) {
  fail_info_ = reason;
}
FailInfo ProtocolMessage::MessageFailInfo() const {
  return fail_info_;
}

void ProtocolMessage::SetIOContext(const RefProtoService& service) {
  io_context_.protocol_service = service;
}

void ProtocolMessage::SetResponse(RefProtocolMessage&& response) {
  response_ = response;
  responsed_ = true;
}

void ProtocolMessage::SetResponse(const RefProtocolMessage& response) {
  response_ = response;
  responsed_ = true;
}

const std::string& ProtocolMessage::FailMessage() const {
  return fail_;
}

void ProtocolMessage::AppendFailMessage(std::string m) {
  if (!fail_.empty()) {
    fail_.append("->").append(m);
    return;
  }
  fail_ = m;
}

const char* ProtocolMessage::MessageTypeStr() const {
  switch(type_) {
    case MessageType::kRequest:
      return "Request Message";
    case MessageType::kResponse:
      return "Response Message";
    default:
      break;
  }
  return "MessageNone";
}

}//end namespace
