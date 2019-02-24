#include "io_buffer.h"
#include "tcp_channel.h"

#include "resp_service.h"
#include "glog/logging.h"
#include "redis_request.h"
#include "protocol/proto_service.h"

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

void RespService::OnDataReceived(const RefTcpChannel &channel, IOBuffer *buffer) {
  CHECK(!IsServerSide());

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

  if (next_incoming_count_ == 0) {

    current_response->SetIOContext(shared_from_this());
    delegate_->OnProtocolMessage(std::static_pointer_cast<ProtocolMessage>(current_response));

    current_response.reset();
    next_incoming_count_ = 0;
  }
}

bool RespService::SendRequestMessage(const RefProtocolMessage &message) {
  if (message->GetMessageType() != MessageType::kRequest) {
    LOG(ERROR) << " only redis client side protocol supported";
    return false;
  }
  CHECK(next_incoming_count_ == 0);

  RedisRequest* request = (RedisRequest*)message.get();
  next_incoming_count_ = request->CmdCount();

  return channel_->Send((uint8_t*)request->body_.data(), request->body_.size()) >= 0;
}

bool RespService::SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) {
	LOG(FATAL) << __FUNCTION__ << " should not reached here, resp only client service supported";
  return false;
};

} //end namespace
