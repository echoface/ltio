#include "io_buffer.h"
#include "tcp_channel.h"

#include "resp_service.h"
#include "glog/logging.h"
#include "redis_request.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

RespService::RespService()
  : ProtoService() {
}

RespService::~RespService() {
}

void RespService::OnStatusChanged(const SocketChannel* channel) {
}

void RespService::OnDataFinishSend(const SocketChannel*) {
}

void RespService::OnDataReceived(const SocketChannel* channel, IOBuffer *buffer) {
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

    current_response->SetIOCtx(shared_from_this());
    delegate_->OnProtocolMessage(RefCast(ProtocolMessage, current_response));

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

}} //end namespace
