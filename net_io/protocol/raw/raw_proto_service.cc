#include <memory>
#include "glog/logging.h"

#include "io_buffer.h"
#include "tcp_channel.h"
#include "raw_message.h"
#include "net_endian.h"
#include "raw_proto_service.h"

namespace lt {
namespace net {

std::atomic<uint64_t> RawProtoService::sequence_id_ = ATOMIC_VAR_INIT(1);

RawProtoService::RawProtoService()
  : ProtoService() {
}

RawProtoService::~RawProtoService() {
  if (timeout_ev_) {
    CHECK(!timeout_ev_->IsAttached());
    delete timeout_ev_;
    timeout_ev_ = nullptr;
  }
}

// override from ProtoService
void RawProtoService::OnStatusChanged(const RefTcpChannel& ch) {
	if (channel_->IsConnected() && timeout_ev_) {
	  IOLoop()->Pump()->AddTimeoutEvent(timeout_ev_);
	}
}

void RawProtoService::OnDataFinishSend(const RefTcpChannel&) {
}

void RawProtoService::OnDataReceived(const RefTcpChannel &channel, IOBuffer *buffer) {
  do {
    // pre calculate frame size
    {
      if (buffer->CanReadSize() < LtRawHeader::kHeaderSize) {
        return;
      }

      const LtRawHeader* predict_header = (const LtRawHeader*)buffer->GetRead();

      uint64_t body_size = predict_header->content_size_;
      if (buffer->CanReadSize() < LtRawHeader::kHeaderSize + body_size) {
        return;
      }
    }

    auto raw_message = LtRawMessage::Create(IsServerSide());
    raw_message->SetIOCtx(shared_from_this());

    LtRawHeader* header = raw_message->MutableHeader();

    ::memcpy(header, buffer->GetRead(), LtRawHeader::kHeaderSize);
    buffer->Consume(LtRawHeader::kHeaderSize);

    raw_message->SetAsyncIdentify(header->identify_id());

    // header heart beat
    if (header->sequence_id_ == LtRawHeader::kHeartBeatId) {
      heart_beat_alive_ = true;  // reset heart beat flag
      if (IsServerSide()) {
        LOG_IF(ERROR, !SendHeartBeat()) << __FUNCTION__ << channel_->ChannelInfo() << " heart beat broken";
      }
      continue;
    }

    if (header->content_size_ > 0) {
      raw_message->AppendContent((char*)buffer->GetRead(), header->content_size_);
      buffer->Consume(header->content_size_);
    }

    if (delegate_) {
      delegate_->OnProtocolMessage(std::static_pointer_cast<ProtocolMessage>(raw_message));
    }
  } while(1);
}

const RefProtocolMessage RawProtoService::NewResponseFromRequest(const RefProtocolMessage &req) {
  CHECK(req->GetMessageType() == MessageType::kRequest);
  return LtRawMessage::Create(false);
}

bool RawProtoService::BeforeSendRequest(LtRawMessage* request)  {
  CHECK(request->GetMessageType() == MessageType::kRequest);
  auto header = request->MutableHeader();
  header->content_size_ = request->ContentLength();
  header->sequence_id_ = sequence_id_++;

  request->SetAsyncIdentify(header->sequence_id_);

  if (LtRawHeader::kHeartBeatId == sequence_id_) {
    sequence_id_++;
  }
  return true;
}

bool RawProtoService::SendRequestMessage(const RefProtocolMessage &message) {
  auto raw_message = static_cast<LtRawMessage*>(message.get());

  BeforeSendRequest(raw_message);
  auto header =  raw_message->MutableHeader();
  if (channel_->Send((const uint8_t*)header, LtRawHeader::kHeaderSize) < 0) {
    return false;
  }

  return channel_->Send((const uint8_t*)raw_message->Content().data(), raw_message->ContentLength()) >= 0;
};

bool RawProtoService::SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) {
  auto raw_request = static_cast<LtRawMessage*>(req.get());
  auto raw_response = static_cast<LtRawMessage*>(res.get());

  LtRawHeader* header = raw_response->MutableHeader();
  header->content_size_ = raw_response->ContentLength();
  header->sequence_id_ = raw_request->Header().sequence_id_;

  VLOG(GLOG_VTRACE) << __FUNCTION__
                    << " request:" << raw_request->Dump()
                    << " response:" << raw_response->Dump();

  if (channel_->Send((uint8_t*)header, LtRawHeader::kHeaderSize) < 0) {;
    return false;
  }

  return channel_->Send((const uint8_t*)raw_response->Content().data(), raw_response->ContentLength()) >= 0;
}

void RawProtoService::AfterChannelClosed() {
  if (timeout_ev_) {
    IOLoop()->Pump()->RemoveTimeoutEvent(timeout_ev_);
  }
}

void RawProtoService::StartHeartBeat(int32_t ms) {
  LOG_IF(ERROR, IsServerSide()) << __FUNCTION__ << " should not keep heart beat on server side";
  if (IsServerSide() || ms < 5) {
    return;
  }
  timeout_ev_ = new base::TimeoutEvent(ms, true);
  timeout_ev_->InstallTimerHandler(NewClosure(std::bind(&RawProtoService::OnHeartBeat, this)));
}

bool RawProtoService::SendHeartBeat() {
  LtRawHeader hb;
  hb.sequence_id_ = LtRawHeader::kHeartBeatId;
  LOG_EVERY_N(INFO, 1000) << __FUNCTION__ << " send heart beat";
  return channel_->Send((const uint8_t*)&hb, sizeof(LtRawHeader)) >= 0;
}

void RawProtoService::OnHeartBeat() {
  DCHECK(!IsServerSide());

  if (!heart_beat_alive_) {
    CloseService();
    return;
  }
  SendHeartBeat();
  heart_beat_alive_ = false;
}

}};
