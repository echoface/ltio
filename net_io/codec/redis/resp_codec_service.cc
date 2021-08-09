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

#include "resp_codec_service.h"
#include <base/base_constants.h>
#include <net_io/codec/codec_service.h>
#include <net_io/io_buffer.h>
#include <net_io/tcp_channel.h>
#include <cstring>
#include "glog/logging.h"
#include "redis_request.h"

namespace lt {
namespace net {

RespCodecService::RespCodecService(base::MessageLoop* loop)
  : CodecService(loop) {}

RespCodecService::~RespCodecService() {}

void RespCodecService::StartProtocolService() {
  StartInternal();

  // Auth && Select DB initialize 
  if (!delegate_ || !delegate_->GetRemoteInfo()) {
    return CodecService::NotifyCodecReady();
  }
  const auto info = delegate_->GetRemoteInfo();

  auto request = std::make_shared<RedisRequest>();
  if (!info->passwd.empty()) {
    request->Auth(info->passwd);
    init_wait_res_flags_ |= InitWaitFlags::kWaitAuth;
  }

  auto db_iter = info->queries.find("db");
  if (db_iter != info->queries.end() && !db_iter->second.empty()) {
    request->Select(db_iter->second);
    init_wait_res_flags_ |= InitWaitFlags::kWaitSelectDB;
  }

  if (request->CmdCount() == 0) {
    return NotifyCodecReady();
  }

  if (!SendRequest(request.get())) {
    init_wait_res_flags_ = InitWaitFlags::kWaitNone;
    // TODO: what should do here
    CloseService();
    return NotifyCodecClosed();
  }
  next_incoming_count_ = request->CmdCount();
}

void RespCodecService::OnDataReceived(IOBuffer* buffer) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";
  CHECK(!IsServerSide());

  if (!current_response) {
    current_response = std::make_shared<RedisResponse>();
  }

  do {
    resp::result res =
        decoder_.decode((const char*)buffer->GetRead(), buffer->CanReadSize());
    if (res != resp::completed) {
      buffer->Consume(res.size());
      break;
    }

    next_incoming_count_--;
    buffer->Consume(res.size());
    current_response->AddResult(res);
  } while (next_incoming_count_ > 0);

  if (next_incoming_count_ == 0) {
    current_response->SetIOCtx(shared_from_this());

    if (handler_ && init_wait_res_flags_ == 0) {
      handler_->OnCodecMessage(std::move(current_response));
    } else if (init_wait_res_flags_ != 0) {
      HandleInitResponse(current_response.get());
      init_wait_res_flags_ = 0;
    }
    current_response.reset();
    next_incoming_count_ = 0;
  }
}

void RespCodecService::HandleInitResponse(RedisResponse* response) {
  uint8_t res_index = 0;
  if (init_wait_res_flags_ & InitWaitFlags::kWaitAuth) {

    init_wait_res_flags_ &= ~(InitWaitFlags::kWaitAuth);

    auto& res_value = response->ResultAtIndex(res_index++);
    if (res_value.type() != resp::ty_string ||
        ::strncmp("OK", res_value.string().data(), 2) != 0) {
      LOG(ERROR) << " redis auth failed";
      return CloseService();
    }
  }
  if (init_wait_res_flags_ & InitWaitFlags::kWaitSelectDB) {
    init_wait_res_flags_ &= ~(InitWaitFlags::kWaitSelectDB);

    auto& res_value = response->ResultAtIndex(res_index++);
    if (res_value.type() != resp::ty_string ||
        ::strncmp("OK", res_value.string().data(), 2) != 0) {
      LOG(ERROR) << " redis select db failed";
      return CloseService();
    }
  }
  NotifyCodecReady();
}

const RefCodecMessage RespCodecService::NewHeartbeat() {
  if (IsServerSide()) {
    return NULL;
  }

  auto request = std::make_shared<RedisRequest>();

  request->AsHeartbeat();
  return RefCast(CodecMessage, request);
}

bool RespCodecService::SendRequest(CodecMessage* message) {
  CHECK(next_incoming_count_ == 0);

  RedisRequest* request = (RedisRequest*)message;
  if (channel_->Send(request->body_.data(), request->body_.size()) >= 0) {
    next_incoming_count_ = request->CmdCount();
    return true;
  }
  return false;
}

bool RespCodecService::SendResponse(const CodecMessage* req,
                                    CodecMessage* res) {
  LOG(FATAL) << __FUNCTION__ << " resp only client service supported";
  return false;
};

}  // namespace net
}  // namespace lt
