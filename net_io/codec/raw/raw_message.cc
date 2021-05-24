/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <endian.h>

#include "raw_message.h"

namespace lt {
namespace net {

const uint8_t LtRawHeader::kHeartbeatMethodId = 0xFF;
const uint64_t LtRawHeader::kHeaderSize = sizeof(LtRawHeader);

LtRawHeader* LtRawHeader::ToNetOrder() {
  sequence_id_ = ::htobe64(sequence_id_);
  content_size_ = ::htobe64(content_size_);
  return this;
}

LtRawHeader* LtRawHeader::FromNetOrder() {
  sequence_id_ = ::be64toh(sequence_id_);
  content_size_ = ::be64toh(content_size_);
  return this;
}

const std::string LtRawHeader::Dump() const {
  std::ostringstream oss;
  oss << "{\"code\": \"" << int(code) << "\","
      << "\"method\": " << int(method) << ","
      << "\"content_sz\": " << content_size_ << ","
      << "\"sequence_id\": \"" << sequence_id_ << "\""
      << "}";
  return std::move(oss.str());
}

LtRawMessage::RefRawMessage RawMessage::Create(bool request) {
  auto t = request ? MessageType::kRequest : MessageType::kResponse;
  return std::make_shared<RawMessage>(t);
}

RawMessage::RefRawMessage RawMessage::CreateResponse(const RawMessage* request) {
  auto response = Create(false);
  response->header_.code = 0;
  response->header_.content_size_ = 0;
  response->header_.method = request->Method();
  response->header_.sequence_id_ = request->AsyncId();
  return response;
}

RawMessage::RefRawMessage RawMessage::Decode(IOBuffer* buffer, bool server_side) {
  if (buffer->CanReadSize() < LtRawHeader::kHeaderSize) {
    return NULL;
  }
  const LtRawHeader* predict_header = (const LtRawHeader*)buffer->GetRead();
  if (buffer->CanReadSize() < predict_header->frame_size()) {
    return NULL;
  }

  //decode
  auto message = Create(server_side);
  ::memcpy(&message->header_, buffer->GetRead(), LtRawHeader::kHeaderSize);
  buffer->Consume(LtRawHeader::kHeaderSize);

  const uint64_t body_size = message->header_.payload_size();
  if (body_size > 0) {
    message->AppendContent((const char*)buffer->GetRead(), body_size);
    buffer->Consume(body_size);
  }
  return message;
}

RawMessage::RawMessage(MessageType t) :
  CodecMessage(t) {
}

bool RawMessage::EncodeTo(SocketChannel* ch) {
  CHECK(content_.size() == header_.payload_size());
  if (ch->Send((const char*)(&header_), LtRawHeader::kHeaderSize) < 0) {
    return false;
  }
  return ch->Send(content_.data(), content_.size()) >= 0;
}

bool RawMessage::AsHeartbeat() {
  SetCode(0);
  SetMethod(LtRawHeader::kHeartbeatMethodId);
  return true;
}

bool RawMessage::IsHeartbeat() const {
  return Code() == 0 && Method() == LtRawHeader::kHeartbeatMethodId;
};

void RawMessage::SetContent(const char* c) {
  content_ = c;
  header_.content_size_ = content_.size();
}

void RawMessage::SetContent(const std::string& c) {
  content_ = c;
  header_.content_size_ = content_.size();
}

void RawMessage::AppendContent(const char* c, uint64_t len) {
  content_.append(c, len);
  header_.content_size_ = content_.size();
}

const std::string RawMessage::Dump() const {
  std::ostringstream oss;
  oss << "{\"type\": \"" << TypeAsStr() << "\","
    << "\"header\": " << header_.Dump() << ","
    << "\"content\": \"" << content_ << "\""
    << "}";
  return std::move(oss.str());
}

}  // namespace net
}  // namespace lt
