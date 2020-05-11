#include "glog/logging.h"

#include "codec_message.h"
#include "codec_service.h"
#include <base/coroutine/coroutine_runner.h>
#include <base/message_loop/message_loop.h>

namespace lt {
namespace net {

const RefCodecMessage CodecMessage::kNullMessage;

CodecMessage::CodecMessage(MessageType type)
  : type_(type),
    fail_code_(kSuccess) {
  work_context_.loop = NULL;
}

CodecMessage::~CodecMessage() {
}

void CodecMessage::SetFailCode(MessageCode reason) {
  fail_code_ = reason;
}
MessageCode CodecMessage::FailCode() const {
  return fail_code_;
}

void CodecMessage::SetIOCtx(const RefCodecService& service) {
  io_context_.codec = service;
  io_context_.io_loop = service->IOLoop();
}

void CodecMessage::SetWorkerCtx(base::MessageLoop* loop) {
  work_context_.loop = loop;
}

void CodecMessage::SetWorkerCtx(base::MessageLoop* loop, StlClosure resumer) {
  work_context_.loop = loop;
  work_context_.resumer_fn = resumer;
}

void CodecMessage::SetResponse(const RefCodecMessage& response) {
  response_ = response;
  responded_ = true;
}

const char* CodecMessage::TypeAsStr() const {
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

}}//end namespace
