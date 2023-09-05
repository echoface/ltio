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
#include "client.h"

#include <algorithm>

#include "base/coroutine/co_runner.h"
#include "base/logging.h"
#include "base/ltio_config.h"
#include "base/utils/string/str_utils.h"
#include "client_channel.h"
#include "net_io/tcp_channel.h"

#ifdef LTIO_HAVE_SSL
#include <memory>
#include "net_io/tcp_channel_ssl.h"
namespace lt {
namespace net {

base::SSLCtx* ClientSSLCtxProvider::GetClientSSLContext() {
  if (ssl_ctx_)
    return ssl_ctx_;
  return &base::SSLCtx::DefaultClientCtx();
}
}  // namespace net
}  // namespace lt
#endif

namespace lt {
namespace net {

static const uint32_t kConnetBatchCount = 10;
const uint32_t Client::kMaxReconInterval = 10000;

Client::Client(base::MessageLoop* loop, const url::RemoteInfo& info)
  : address_(info.host_ip, info.port),
    remote_info_(info),
    work_loop_(loop),
    stopping_(false),
    delegate_(NULL) {
  next_index_ = 0;
  CHECK(work_loop_);

  auto empty_list = std::make_shared<ClientChannelList>();
  std::atomic_store(&in_use_channels_, empty_list);

  channels_count_.store(0);
  connector_.reset(new Connector(work_loop_->Pump()));
}

Client::~Client() {
  Finalize();
  VLOG(VINFO) << __func__ << " gone:" << ClientInfo();
}

void Client::SetDelegate(ClientDelegate* delegate) {
  delegate_ = delegate;
}

void Client::Initialize(const ClientConfig& config) {
  config_ = config;
  uint32_t init_count = std::min(kConnetBatchCount, required_count());
  for (uint32_t i = 0; i < init_count; i++) {
    work_loop_->PostTask(FROM_HERE,
                         &Connector::Dial,
                         connector_,
                         address_,
                         this);
  }
}

void Client::Finalize() {
  if (stopping_.exchange(true)) {
    return;
  }

  channels_count_ = 0;
  work_loop_->PostTask(FROM_HERE, &Connector::Stop, connector_);

  auto channels = std::atomic_load(&in_use_channels_);
  for (RefClientChannel& ch : *channels) {
    base::MessageLoop* loop = ch->IOLoop();
    // this close will success and client detach with it,
    // no any notify callback will be called any more
    // in this io_loop, it will close all resource of this transport
    loop->PostTask(FROM_HERE, &ClientChannel::CloseClientChannel, ch);
  }
}

void Client::OnConnectFailed(uint32_t count) {
  CHECK(work_loop_->IsInLoopThread());

  VLOG(VTRACE) << __FUNCTION__ << " connect failed";
  if (stopping_ || required_count() <= ConnectedCount()) {
    return;
  }

  // re-connct
  next_reconnect_interval_ += 10;

  if (connector_->InprocessCount() > kConnetBatchCount) {
    VLOG(VERROR) << RemoteIpPort() << ", giveup reconnect"
                 << ", inprogress connection:" << connector_->InprocessCount();
  } else {
    int32_t delay = std::min(next_reconnect_interval_, kMaxReconInterval);
    auto functor = std::bind(&Connector::Dial, connector_, address_, this);
    work_loop_->PostDelayTask(NewClosure(functor), delay);
    VLOG(VERROR) << "reconnect:" << RemoteIpPort() << " after " << delay
                 << "(ms)";
  }
}

base::MessageLoop* Client::next_client_io_loop() {
  base::MessageLoop* io_loop = NULL;
  if (delegate_) {
    io_loop = delegate_->NextIOLoopForClient();
  }
  return io_loop ? io_loop : work_loop_;
}

uint32_t Client::required_count() const {
  return config_.connections;
}

void Client::launch_next_if_need() {
  CHECK(work_loop_->IsInLoopThread());

  uint64_t connected = ConnectedCount();
  uint32_t inprocess_cnt = connector_->InprocessCount();
  if (stopping_ || required_count() <= connected + inprocess_cnt) {
    return;
  }
  int start = connected + inprocess_cnt;
  work_loop_->PostTask(FROM_HERE, &Connector::Dial, connector_, address_, this);
}

RefClientChannel Client::get_ready_channel() {
  auto channels = std::atomic_load(&in_use_channels_);
  if (!channels || channels->size() == 0)
    return NULL;

  int32_t max_step = std::min(10, int(channels->size()));
  do {
    uint32_t idx = next_index_.fetch_add(1) % channels->size();
    auto& ch = channels->at(idx);
    if (ch && ch->Ready()) {
      return ch;
    }
  } while (max_step--);
  return NULL;
}

void Client::OnConnected(int socket_fd, IPEndPoint& local, IPEndPoint& remote) {
  CHECK(work_loop_->IsInLoopThread());
  next_reconnect_interval_ = 0;

  base::MessageLoop* io_loop = next_client_io_loop();

  // create socket fdevent
  auto fdev = base::FdEvent::Create(nullptr, socket_fd, base::LtEv::READ);

  // create codec service
  RefCodecService codec =
      CodecFactory::NewClientService(remote_info_.scheme, io_loop);
  CHECK(codec) << "no supported codec for:" << remote_info_.scheme;

  RefClientChannel client_channel = CreateClientChannel(this, codec);
  client_channel->SetRequestTimeout(config_.message_timeout);

  SocketChannelPtr channel;
  if (codec->UseSSLChannel()) {
#ifdef LTIO_HAVE_SSL
    auto ch = TCPSSLChannel::Create(socket_fd, local, remote);
    ch->InitSSL(GetClientSSLContext()->NewSSLSession(socket_fd));
    channel = std::move(ch);
#else
    CHECK(false) << "ssl need compile with openssl lib support";
#endif
  } else {
    channel = TcpChannel::Create(socket_fd, local, remote);
  }
  codec->BindSocket(std::move(fdev), std::move(channel));

  channels_.push_back(client_channel);
  io_loop->PostTask(FROM_HERE,
                    &ClientChannel::StartClientChannel,
                    client_channel);

  RefClientChannelList new_list(
      new ClientChannelList(channels_.begin(), channels_.end()));
  channels_count_.store(new_list->size());
  std::atomic_store(&in_use_channels_, new_list);
  VLOG(VINFO) << ClientInfo() << " connected, initializing...";

  launch_next_if_need();
}

void Client::OnClientChannelInited(const ClientChannel* channel) {
  if (!stopping_ && !work_loop_->IsInLoopThread()) {
    auto thisfunc = std::bind(&Client::OnClientChannelInited, this, channel);
    work_loop_->PostTask(NewClosure(thisfunc));
    return;
  }

  if (delegate_) {
    delegate_->OnClientChannelReady(channel);
  }
  VLOG(VINFO) << ClientInfo() << "@" << channel << " ready for use";
}

/*on the loop of client IO, need managed by connector loop*/
void Client::OnClientChannelClosed(const RefClientChannel& channel) {
  if (!work_loop_->IsInLoopThread() && !stopping_) {
    auto thisfunc = std::bind(&Client::OnClientChannelClosed, this, channel);
    work_loop_->PostTask(NewClosure(std::move(thisfunc)));
    return;
  }

  VLOG(VINFO) << ClientInfo() << "@" << channel.get() << " closed";

  do {
    channels_.remove_if([&](const RefClientChannel& ch) -> bool {
      return ch == NULL || ch == channel;
    });

    RefClientChannelList new_list(
        new ClientChannelList(channels_.begin(), channels_.end()));
    channels_count_.store(new_list->size());

    std::atomic_store(&in_use_channels_, new_list);
  } while (0);

  // reconnect
  launch_next_if_need();

  uint32_t connected_count = channels_.size();
  if (!stopping_ && connected_count == 0 && delegate_) {
    delegate_->OnAllClientPassiveBroken(this);
  }
}

void Client::OnRequestGetResponse(const RefCodecMessage& request,
                                  const RefCodecMessage& response) {
  request->SetResponse(response);
  request->GetWorkCtx().resumer_fn();
}

bool Client::AsyncDoRequest(const RefCodecMessage& req,
                            AsyncCallBack callback) {
  base::MessageLoop* worker = next_client_io_loop();
  CHECK(worker);

  // IMPORTANT: avoid self holder for capture list
  CodecMessage* request = req.get();

  base::LtClosure resumer = [=]() {
    if (worker->IsInLoopThread()) {
      callback(request->RawResponse());
    } else {
      auto responser = std::bind(callback, request->RawResponse());
      worker->PostTask(FROM_HERE, responser);
    }
  };

  req->SetWorkerCtx(worker, std::move(resumer));

  RefClientChannel client = get_ready_channel();
  if (!client) {
    return false;
  }

  base::MessageLoop* io = client->IOLoop();
  return io->PostTask(FROM_HERE, &ClientChannel::SendRequest, client, req);
}

CodecMessage* Client::DoRequest(RefCodecMessage& message) {
  CHECK(CO_CANYIELD);

  message->SetWorkerCtx(base::MessageLoop::Current(), CO_RESUMER);

  auto channel = get_ready_channel();
  if (!channel) {
    message->SetFailCode(MessageCode::kNotConnected);
    LOG_EVERY_N(ERROR, 1000)
        << ClientInfo() << ", no inited/established client";
    return NULL;
  }

  base::MessageLoop* io_loop = channel->IOLoop();
  if (!io_loop->PostTask(FROM_HERE,
                         &ClientChannel::SendRequest,
                         channel,
                         message)) {
    message->SetFailCode(MessageCode::kConnBroken);
    return NULL;
  }

  CO_YIELD;

  return message->RawResponse();
}

uint64_t Client::ConnectedCount() const {
  return channels_count_;
}

std::string Client::ClientInfo() const {
  std::ostringstream oss;
  oss << "[remote:" << RemoteIpPort() << ", in_use:" << ConnectedCount()
      << ", connecting:" << connector_->InprocessCount() << "]";
  return oss.str();
}

std::string Client::RemoteIpPort() const {
  return base::StrUtil::Concat(remote_info_.host_ip, ":", remote_info_.port);
}

}  // namespace net
}  // namespace lt
