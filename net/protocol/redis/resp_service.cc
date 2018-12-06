#include "io_buffer.h"
#include "resp_service.h"
#include "glog/logging.h"
#include "redis_request.h"
#include "protocol/proto_service.h"
#include "net/tcp_channel.h"

namespace net {

RespService::RespService()
  : ProtoService() {
}

RespService::~RespService() {
}

void RespService::OnStatusChanged(const RefTcpChannel& channel) {
}

void RespService::OnDataFinishSend(const RefTcpChannel&) {
}

void RespService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buffer) {
  CHECK(ProtoService::ServiceType() == ProtocolServiceType::kClient);

  if (!current_response) {
    current_response = std::make_shared<RedisResponse>();
  }

  do {

    resp::result res = decoder_.decode((const char*)buffer->GetRead(), buffer->CanReadSize());
    if (res != resp::completed) {
      buffer->Consume(res.size());
      break;
    }

    next_incoming_count_--;
    buffer->Consume(res.size());
    current_response->AddResult(res);
  } while(next_incoming_count_ > 0);

  if (next_incoming_count_ == 0 && message_handler_) {
    message_handler_(std::static_pointer_cast<ProtocolMessage>(current_response));
    current_response.reset();
    next_incoming_count_ = 0;
  }
}

bool RespService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  if (msg->GetMessageType() != MessageType::kRequest) {
    LOG(ERROR) << " only redis client side protocol supported";
    return false;
  }

  RedisRequest* request = (RedisRequest*)msg;
  out_buffer->EnsureWritableSize(request->body_.size());
  out_buffer->WriteRawData(request->body_.data(), request->body_.size());

  return true;
}

bool RespService::SendProtocolMessage(RefProtocolMessage& message) {
  if (message->GetMessageType() != MessageType::kRequest) {
    LOG(ERROR) << " only redis client side protocol supported";
    return false;
  }
  BeforeWriteMessage(message.get());
  RedisRequest* request = (RedisRequest*)message.get();
  return channel_->Send((uint8_t*)request->body_.data(), request->body_.size()) >= 0;
}

void RespService::BeforeWriteRequestToBuffer(ProtocolMessage* out_message) {
  RedisRequest* req = (RedisRequest*)out_message;
  //save pipeline count
  next_incoming_count_ = req->CmdCount();
}

void RespService::BeforeWriteResponseToBuffer(ProtocolMessage* out_message) {
  LOG(ERROR) << "redis server side not implemented. should not reach here";
}

void RespService::BeforeSendResponse(ProtocolMessage *in, ProtocolMessage *out) {
}

} //end namespace
