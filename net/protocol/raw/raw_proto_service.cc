
#include <memory>
#include "io_buffer.h"
#include "tcp_channel.h"
#include "glog/logging.h"
#include "raw_message.h"
#include "raw_proto_service.h"

namespace net {

static const uint32_t kRawHeaderSize = sizeof(RawHeader);
RawProtoService::RawProtoService()
  : ProtoService() {
}

RawProtoService::~RawProtoService() {
}

// override from ProtoService
void RawProtoService::OnStatusChanged(const RefTcpChannel&) {
}

void RawProtoService::OnDataFinishSend(const RefTcpChannel&) {
}

void RawProtoService::OnDataReceived(const RefTcpChannel &channel, IOBuffer *buffer) {
  do {
    if (buffer->CanReadSize() <= (int)kRawHeaderSize) {
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

    std::string& content = raw_message->MutableContent();
    content.append((const char*)buffer->GetRead(), body_size);
    buffer->Consume(body_size);
    if (message_handler_) {
      LOG(ERROR) << " raw service got message:" << raw_message->MessageDebug();
      message_handler_(std::static_pointer_cast<ProtocolMessage>(raw_message));
    }
  } while(1);
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

  if (sequence_id_ == 0) {
    sequence_id_++;
  }
  return true;
}

bool RawProtoService::SendRequestMessage(const RefProtocolMessage &message) {
  auto raw_message = static_cast<RawMessage*>(message.get());

  BeforeSendRequest(raw_message);

  CHECK(raw_message->header_.frame_size == kRawHeaderSize + raw_message->content_.size());

  if (channel_->Send((const uint8_t*)&raw_message->header_, kRawHeaderSize) < 0) {;
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
                    << " request:" << raw_request->MessageDebug()
                    << " response:" << raw_response->MessageDebug();

  return channel_->Send((const uint8_t*)raw_response->content_.data(), raw_response->content_.size()) >= 0;
}

};
