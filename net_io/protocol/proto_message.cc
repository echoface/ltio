#include "glog/logging.h"
#include "proto_message.h"
#include "proto_service.h"

namespace net {

const RefProtocolMessage ProtocolMessage::kNullMessage = RefProtocolMessage();

ProtocolMessage::ProtocolMessage(MessageType type)
  : type_(type),
    fail_code_(kSuccess),
    responded_(false) {
  work_context_.loop = NULL;
}

ProtocolMessage::~ProtocolMessage() {
}

void ProtocolMessage::SetFailCode(MessageCode reason) {
  fail_code_ = reason;
}
MessageCode ProtocolMessage::FailCode() const {
  return fail_code_;
}

void ProtocolMessage::SetIOCtx(const RefProtoService& service) {
  io_context_.protocol_service = service;
  io_context_.io_loop = service->IOLoop();
}

void ProtocolMessage::SetWorkerCtx(base::MessageLoop* loop) {
  work_context_.loop = loop;
}

void ProtocolMessage::SetWorkerCtx(base::MessageLoop* loop, base::StlClosure coro_resumer) {
  work_context_.loop = loop;
  work_context_.resumer_fn = coro_resumer;
}

void ProtocolMessage::SetResponse(const RefProtocolMessage& response) {
  response_ = response;
  responded_ = true;
}

const char* ProtocolMessage::TypeAsStr() const {
  switch(type_) {
    case MessageType::kRequest:
      return "Request";
    case MessageType::kResponse:
      return "Response";
    default:
      break;
  }
  return "MessageNone";
}

}//end namespace
