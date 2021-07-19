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

//
// Created by gh on 18-12-15.
//

#include "client_channel.h"
#include <base/utils/string/str_utils.h>
#include "async_channel.h"
#include "queued_channel.h"

namespace lt {
namespace net {

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate,
                                     RefCodecService service) {
  if (service->KeepSequence()) {
    auto client_channel = QueuedChannel::Create(delegate, service);
    return std::static_pointer_cast<ClientChannel>(client_channel);
  }
  auto client_channel = AsyncChannel::Create(delegate, service);
  return std::static_pointer_cast<ClientChannel>(client_channel);
}

ClientChannel::ClientChannel(Delegate* d, const RefCodecService& service)
  : delegate_(d), codec_(service) {
  codec_->SetDelegate(this);
}

ClientChannel::~ClientChannel() {
  VLOG(GLOG_VTRACE) << __func__ << " this:" << this << " gone";
  codec_->SetDelegate(NULL);
}

void ClientChannel::StartClientChannel() {
  codec_->StartProtocolService();
}

void ClientChannel::CloseClientChannel() {
  base::EventPump* pump = EventPump();
  CHECK(pump->IsInLoop());

  VLOG(GLOG_VTRACE) << " going to close:" << ConnectionInfo();

  state_ = kClosing;

  BeforeCloseChannel();

  delegate_ = NULL;

  codec_->CloseService(true);

  OnProtocolServiceGone(codec_);

  //fallback avlid inherit class skip
  //ClientChannel::OnProtocolServiceGone
  if (heartbeat_timer_) {
    pump->RemoveTimeoutEvent(heartbeat_timer_);
    delete heartbeat_timer_;
    heartbeat_timer_ = NULL;
  }

  state_ = kDisconnected;
  VLOG(GLOG_VTRACE) << " close:" << ConnectionInfo() << " back";
}

std::string ClientChannel::ConnectionInfo() const {
  const url::RemoteInfo* remote_info = GetRemoteInfo();
  if (remote_info) {
    return remote_info->HostIpPort();
  }
  return std::string("unknown connection");
}

// override
const url::RemoteInfo* ClientChannel::GetRemoteInfo() const {
  if (delegate_) {
    return &(delegate_->GetRemoteInfo());
  }
  return NULL;
}

void ClientChannel::OnHearbeatTimerInvoke() {
  if (heartbeat_message_) {
    LOG(ERROR) << __FUNCTION__ << " heartbea snowball";
    return;
  }
  heartbeat_message_ = codec_->NewHeartbeat();
  return SendRequest(heartbeat_message_);
}

bool ClientChannel::HandleResponse(const RefCodecMessage& req,
                                   const RefCodecMessage& res) {
  if (heartbeat_message_.get() == req.get()) {
    LOG_IF(INFO, !res) << "heartbeat got null response, code:" << req->FailCode();
    heartbeat_message_.reset();
    if (res && !res->IsHeartbeat()) {
      LOG(ERROR) << "heartbeat message got none heartbeat response";
    }
    return true;
  }

  if (delegate_) {
    delegate_->OnRequestGetResponse(req, res);
    return true;
  }

  return false;
}

// override
void ClientChannel::OnProtocolServiceGone(const RefCodecService& service) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";

  if (heartbeat_timer_) {
    EventPump()->RemoveTimeoutEvent(heartbeat_timer_);
    delete heartbeat_timer_;
    heartbeat_timer_ = NULL;
  }
}

// override
void ClientChannel::OnProtocolServiceReady(const RefCodecService& service) {
  state_ = kReady;
  uint32_t heartbeat_ms = 0;
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ConnectionInfo();

  if (delegate_) {
    delegate_->OnClientChannelInited(this);
    heartbeat_ms = delegate_->GetClientConfig().heartbeat_ms;
  }

  if (codec_->KeepHeartBeat() && heartbeat_ms > 50) {
    heartbeat_timer_ = new base::TimeoutEvent(heartbeat_ms, true);

    auto heartbeat_fun = std::bind(&ClientChannel::OnHearbeatTimerInvoke, this);
    heartbeat_timer_->InstallHandler(NewClosure(heartbeat_fun));
    EventPump()->AddTimeoutEvent(heartbeat_timer_);
  }
};

}  // namespace net
}  // namespace lt
