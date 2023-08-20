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

#include "codec_message.h"

#include "base/message_loop/message_loop.h"
#include "codec_service.h"

namespace lt {
namespace net {

const RefCodecMessage CodecMessage::kNullMessage;

CodecMessage::CodecMessage() : code_(kSuccess) {
  work_context_.loop = NULL;
}

CodecMessage::~CodecMessage() {}

void CodecMessage::SetTimeout() {
  code_ = kTimeOut;
}

void CodecMessage::SetFailCode(MessageCode reason) {
  code_ = reason;
}

MessageCode CodecMessage::FailCode() const {
  return code_;
}

void CodecMessage::SetIOCtx(const RefCodecService& service) {
  io_context_.codec = service;
  io_context_.io_loop = service->IOLoop();
}

void CodecMessage::SetWorkerCtx(base::MessageLoop* loop) {
  work_context_.loop = loop;
}

void CodecMessage::SetWorkerCtx(base::MessageLoop* loop,
                                base::LtClosure resumer) {
  work_context_.loop = loop;
  work_context_.resumer_fn = resumer;
}

void CodecMessage::SetResponse(const RefCodecMessage& response) {
  response_ = response;
}

}  // namespace net
}  // namespace lt
