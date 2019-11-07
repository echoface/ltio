#include "io_buffer.h"
#include "tcp_channel.h"

#include <cstring>
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

    if (delegate_ && init_wait_res_flags_ == 0) {
      delegate_->OnProtocolMessage(RefCast(ProtocolMessage, current_response));
    } else if (init_wait_res_flags_ != 0) {
      HandleInitResponse(current_response.get());
      init_wait_res_flags_ = 0;
    }
    current_response.reset();
    next_incoming_count_ = 0;
  }
}

void RespService::HandleInitResponse(RedisResponse* response) {
  uint8_t res_index = 0;
  if (init_wait_res_flags_ & InitWaitFlags::kWaitAuth) {
    init_wait_res_flags_ &= ~(InitWaitFlags::kWaitAuth);

    auto& res_value = response->ResultAtIndex(res_index++);
    if (res_value.type() != resp::ty_string || 0 != ::strncmp("OK", res_value.string().data(), 2)) {
      LOG(ERROR) << " redis auth failed";
      return CloseService();
    }
  }
  if (init_wait_res_flags_ & InitWaitFlags::kWaitSelectDB) {
    init_wait_res_flags_ &= ~(InitWaitFlags::kWaitSelectDB);

    auto& res_value = response->ResultAtIndex(res_index++);
    if (res_value.type() != resp::ty_string || 0 != ::strncmp("OK", res_value.string().data(), 2)) {
      LOG(ERROR) << " redis select db failed";
      return CloseService();
    }
  }
  ProtoService::OnChannelReady(channel_.get());
}

// do initialize for redis protocol, auth && select db
void RespService::OnChannelReady(const SocketChannel* ch) {
  if (!delegate_ || !delegate_->GetRemoteInfo()) {
    return ProtoService::OnChannelReady(ch);
  }
  const url::RemoteInfo* info = delegate_->GetRemoteInfo();

  auto request = std::make_shared<RedisRequest>();

  if (!info->passwd.empty()) {
    request->Auth(info->passwd);
    init_wait_res_flags_ |= InitWaitFlags::kWaitAuth;
  }

  auto db_iter = info->querys.find("db");
  if (db_iter != info->querys.end() && !db_iter->second.empty()) {
    request->Select(db_iter->second);
    init_wait_res_flags_ |= InitWaitFlags::kWaitSelectDB;
  }

  if (request->CmdCount() == 0) {
    return ProtoService::OnChannelReady(ch);
  }

  if (!SendRequestMessage(RefCast(ProtocolMessage, request))) {
    return CloseService();
  }
  next_incoming_count_ = request->CmdCount();
}

bool RespService::SendRequestMessage(const RefProtocolMessage &message) {
  if (message->GetMessageType() != MessageType::kRequest) {
    LOG(ERROR) << " only redis client side protocol supported";
    return false;
  }
  CHECK(next_incoming_count_ == 0);

  RedisRequest* request = (RedisRequest*)message.get();
  if (channel_->Send(request->body_.data(), request->body_.size()) >= 0) {
    next_incoming_count_ = request->CmdCount();
    return true;
  }
  return false;
}

bool RespService::SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) {
	LOG(FATAL) << __FUNCTION__ << " should not reached here, resp only client service supported";
  return false;
};

}} //end namespace
