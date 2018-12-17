
#include <memory>
#include "io_buffer.h"
#include "tcp_channel.h"
#include "glog/logging.h"
#include "raw_message.h"
#include "raw_proto_service.h"

namespace net {

#define kRawHeartBeatId 0

static const uint32_t kRawHeaderSize = sizeof(RawHeader);
std::atomic<uint64_t> RawProtoService::sequence_id_ = ATOMIC_VAR_INIT(1);

RawProtoService::RawProtoService()
  : ProtoService() {
}

RawProtoService::~RawProtoService() {
  {
    CHECK(!timeout_ev_->IsAtatched());
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
    if (buffer->CanReadSize() < (uint64_t)kRawHeaderSize) {
      return;
    }

    const RawHeader* header = (const RawHeader*)buffer->GetRead();
    uint32_t frame_size = header->frame_size;
    int32_t body_size = frame_size - kRawHeaderSize;

    if (buffer->CanReadSize() < frame_size) {
      return;
    }

    auto raw_message = std::make_shared<RawMessage>(InComingType());
    raw_message->SetIOContext(shared_from_this());

    memcpy(&(raw_message->header_), buffer->GetRead(), kRawHeaderSize);
    buffer->Consume(kRawHeaderSize);

    if (body_size > 0) {
      std::string& content = raw_message->MutableContent();
      content.append((const char*)buffer->GetRead(), body_size);
      buffer->Consume(body_size);
    }

    HandleMessage(raw_message);
  } while(1);
}

void RawProtoService::HandleMessage(RefRawMessage& message) {

  if (message->SequenceId() == kRawHeartBeatId) {
    heart_beat_alive_ = true;
    if (IsServerService()) {
      if (channel_->Send((const uint8_t*)&message->header_, kRawHeaderSize) < 0) {
        LOG(ERROR) << __FUNCTION__ << channel_->ChannelInfo() << " heart beat broken";
      }
    }
    return;
  }

  if (message_handler_) {
    message_handler_(std::static_pointer_cast<ProtocolMessage>(message));
  }
}

bool RawProtoService::CloseAfterMessage(ProtocolMessage*, ProtocolMessage*) {
  return false;
}

const RefProtocolMessage RawProtoService::NewResponseFromRequest(const RefProtocolMessage &req) {
  CHECK(req->GetMessageType() == MessageType::kRequest);
  return std::make_shared<RawMessage>(MessageType::kResponse);
}


bool RawProtoService::BeforeSendRequest(RawMessage* request)  {
	CHECK(request->GetMessageType() == MessageType::kRequest);

  request->CalculateFrameSize();
  request->SetSequenceId(sequence_id_++);
  if (sequence_id_ == kRawHeartBeatId) {
    sequence_id_++;
  }
  return true;
}

bool RawProtoService::SendRequestMessage(const RefProtocolMessage &message) {
  auto raw_message = static_cast<RawMessage*>(message.get());

  BeforeSendRequest(raw_message);

  CHECK(raw_message->header_.frame_size == kRawHeaderSize + raw_message->content_.size());

  if (channel_->Send((const uint8_t*)&raw_message->header_, kRawHeaderSize) < 0) {
    return false;
  }

  return channel_->Send((const uint8_t*)raw_message->content_.data(), raw_message->content_.size()) >= 0;
};

bool RawProtoService::ReplyRequest(const RefProtocolMessage& req, const RefProtocolMessage& res) {
  RawMessage* raw_request = static_cast<RawMessage*>(req.get());
  RawMessage* raw_response = static_cast<RawMessage*>(res.get());

  raw_response->CalculateFrameSize();
  raw_response->SetMethod(raw_request->Method());
  raw_response->SetSequenceId(raw_request->SequenceId());

  VLOG(GLOG_VTRACE) << __FUNCTION__
                    << " request:" << raw_request->Dump()
                    << " response:" << raw_response->Dump();

  if (channel_->Send((const uint8_t*)&raw_response->header_, kRawHeaderSize) < 0) {;
    return false;
  }

  return channel_->Send((const uint8_t*)raw_response->content_.data(), raw_response->content_.size()) >= 0;
}

void RawProtoService::AfterChannelClosed() {
  if (timeout_ev_) {
    IOLoop()->Pump()->RemoveTimeoutEvent(timeout_ev_);
  }
}

void RawProtoService::StartHeartBeat(int32_t ms) {
	if (IsServerService() || ms < 5) {
	  LOG_IF(ERROR, IsServerService()) << __FUNCTION__ << " should not keep heart beat on server side";
	  return;
	}

  timeout_ev_ = new base::TimeoutEvent(ms, true);
  timeout_ev_->InstallTimerHandler(NewClosure(std::bind(&RawProtoService::OnHeartBeat, this)));
}

void RawProtoService::OnHeartBeat() {

	DCHECK(!IsServerService());
  static RawHeader hb;
  hb.sequence_id = kRawHeartBeatId;

  if (!heart_beat_alive_) {
	  CloseService();
    return;
  }
  LOG_EVERY_N(INFO, 1000) << __FUNCTION__ << " send heart beat";
  channel_->Send((const uint8_t*)&hb, sizeof(RawHeader));
  heart_beat_alive_ = false;
}

};
