
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

void RawProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buffer) {
  do {
    if (buffer->CanReadSize() <= (int)kRawHeaderSize) {
      return;
    }
    const RawHeader* header = (const RawHeader*)buffer->GetRead();
    int32_t frame_size = header->frame_size;
    int32_t body_size = frame_size - kRawHeaderSize;

    if (buffer->CanReadSize() < frame_size) {
      return;
    }

    auto raw_message = std::make_shared<RawMessage>(InComingMessageType());

    raw_message->SetIOContextWeakChannel(channel);

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

bool RawProtoService::BeforeSendRequest(ProtocolMessage *message)  {
  CHECK(ServiceType() == ProtocolServiceType::kClient);
  RawMessage* request = static_cast<RawMessage*>(message);
  request->SetSequenceId(sequence_id_);
  sequence_id_++;
  if (sequence_id_ == 0) {
    sequence_id_++;
  }
  return true;
}

bool RawProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  CHECK(msg && out_buffer);

  const RawMessage* raw_message = static_cast<const RawMessage*>(msg);
  CHECK(raw_message->header_.frame_size == kRawHeaderSize + raw_message->content_.size());

  out_buffer->EnsureWritableSize(raw_message->header_.frame_size);

  out_buffer->WriteRawData(&raw_message->header_, kRawHeaderSize);
  out_buffer->WriteString(raw_message->content_);
  return true;
}

bool RawProtoService::SendProtocolMessage(RefProtocolMessage& message) {

  const RawMessage* raw_message = static_cast<const RawMessage*>(message.get());
  CHECK(raw_message->header_.frame_size == kRawHeaderSize + raw_message->content_.size());

  if (channel_->Send((const uint8_t*)&raw_message->header_, kRawHeaderSize) < 0) {;
    return false;
  }

  return channel_->Send((const uint8_t*)raw_message->content_.data(), raw_message->content_.size()) >= 0;
};


void RawProtoService::BeforeSendResponse(ProtocolMessage *in, ProtocolMessage *out) {
  CHECK(IsServerSideservice());
  RawMessage* raw_request = (RawMessage*)in;
  RawMessage* raw_response = (RawMessage*)out;
  raw_response->SetSequenceId(raw_request->Header().sequence_id);
  LOG(ERROR) << "request:" << raw_request->MessageDebug() << " response:" << raw_response->MessageDebug();
}

bool RawProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  return false;
}

const RefProtocolMessage RawProtoService::NewResponseFromRequest(const RefProtocolMessage &req) {
  CHECK(req->GetMessageType() == MessageType::kRequest);
  return std::make_shared<RawMessage>(MessageType::kResponse);
}

};
