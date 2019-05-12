#include "client.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_runner.h"

namespace net {

Client::Client(base::MessageLoop* loop, const url::SchemeIpPort& info)
  : address_(info.host_ip, info.port),
    server_info_(info),
    work_loop_(loop),
    is_stopping_(false),
    delegate_(NULL) {

  router_counter_ = 0;
  CHECK(work_loop_);
  connector_ = std::make_shared<Connector>(work_loop_, this);

  std::shared_ptr<ClientChannelList> list(new ClientChannelList(channels_));
  std::atomic_store(&roundrobin_channes_, list);
}

Client::~Client() {
}

void Client::SetDelegate(RouterDelegate* delegate) {
  delegate_ = delegate;
}

void Client::SetupRouter(const RouterConf& config) {
  config_ = config;
}

void Client::StartRouter() {
  for (uint32_t i = 0; i < config_.connections; i++) {
    auto functor = std::bind(&Connector::Launch, connector_, address_);
    work_loop_->PostTask(NewClosure(functor));
  }
}

void Client::StopRouter() {
  auto channels = std::atomic_load(&roundrobin_channes_);
  if (is_stopping_) {
    return;
  }
  is_stopping_ = true;
  for (auto& client : *channels) {
    client->CloseClientChannel();
  }
}

void Client::StopRouterSync() {

  StopRouter();

  std::unique_lock<std::mutex> lck(mtx_);
  cv_.wait(lck, [&]{
    auto channels = std::atomic_load(&roundrobin_channes_);
    return channels->empty();
  });
}

void Client::OnClientConnectFailed() {
  CHECK(work_loop_->IsInLoopThread());
  if (is_stopping_ || config_.connections <= channels_.size()) {
    return;
  }
  auto functor = std::bind(&Connector::Launch, connector_, address_);
  work_loop_->PostDelayTask(NewClosure(functor), config_.recon_interval);
  VLOG(GLOG_VERROR) << __FUNCTION__ << RouterInfo() << " try after " << config_.recon_interval << " ms";
}

base::MessageLoop* Client::GetLoopForClient() {
  base::MessageLoop* io_loop = NULL;
  if (delegate_) {
    io_loop = delegate_->NextIOLoopForClient();
  }
  return io_loop ? io_loop : work_loop_;
}

void Client::OnNewClientConnected(int socket_fd, SocketAddress& local, SocketAddress& remote) {
  CHECK(work_loop_->IsInLoopThread());

  base::MessageLoop* io_loop = GetLoopForClient();

  auto proto_service = ProtoServiceFactory::Create(server_info_.protocol, false);

  proto_service->BindChannel(socket_fd, local, remote, io_loop);

  auto client_channel = CreateClientChannel(this, proto_service);

  client_channel->SetRequestTimeout(config_.message_timeout);

  client_channel->StartClient();

  channels_.push_back(client_channel);
  RefClientChannelList list(new ClientChannelList(channels_));

  std::atomic_store(&roundrobin_channes_, list);
  if (delegate_) {
    delegate_->OnClientChannelReady(client_channel.get());
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << RouterInfo() << " new protocol service started";
}

/*on the loop of client IO, need managed by connector loop*/
void Client::OnClientChannelClosed(const RefClientChannel& channel) {
  if (!work_loop_->IsInLoopThread()) {
    auto functor = std::bind(&Client::OnClientChannelClosed, this, channel);
    work_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  auto iter = std::find(channels_.begin(), channels_.end(), channel);
  if (iter != channels_.end()) {
    channels_.erase(iter);
  }

  std::shared_ptr<ClientChannelList> list(new ClientChannelList(channels_));
  std::atomic_store(&roundrobin_channes_, list);

  if (!is_stopping_ && channels_.size() < config_.connections) {

    auto f = std::bind(&Connector::Launch, connector_, address_);
    work_loop_->PostDelayTask(NewClosure(f), config_.recon_interval);

    VLOG(GLOG_VTRACE) << __FUNCTION__ << RouterInfo() << " reconnect after:" << config_.recon_interval;
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << RouterInfo();

  if (is_stopping_ && channels_.empty()) {//for sync stopping
    cv_.notify_all();
  }
}

void Client::OnRequestGetResponse(const RefProtocolMessage& request,
                                        const RefProtocolMessage& response) {
  request->SetResponse(response);
  request->GetWorkCtx().resumer_fn();
}

bool Client::AsyncSendRequest(RefProtocolMessage& req, AsyncCallBack callback) {
  base::MessageLoop* worker = base::MessageLoop::Current();
  if (!worker) {
    LOG(ERROR) << " not in a MessageLoop, can't send request here:";
    return false;
  }

  //!!!! avoid self holder
  ProtocolMessage* raw_request = req.get();
  base::StlClosure resumer = [=]() {
    if (worker->IsInLoopThread()) {
      callback(raw_request->RawResponse());
      return;
    }
    worker->PostTask(NewClosure(std::bind(callback, raw_request->RawResponse())));
  };

  req->SetWorkerCtx(worker, std::move(resumer));

  req->SetRemoteHost(server_info_.host);

  auto channels = std::atomic_load(&roundrobin_channes_);
  if (channels->empty()) { //avoid x/0 Error
    LOG(ERROR) << " No Connection Established To Server:" << address_.IpPort();
    return false;
  }
  uint32_t client_index = router_counter_.fetch_add(1) % channels->size();

  RefClientChannel& client = channels->at(client_index);
  base::MessageLoop* io = client->IOLoop();

  return io->PostTask(NewClosure(std::bind(&ClientChannel::SendRequest, client, req)));
}

ProtocolMessage* Client::SendClientRequest(RefProtocolMessage& message) {

  if (!base::MessageLoop::Current() || !base::CoroRunner::CanYield()) {
    LOG(ERROR) << __FUNCTION__ << " must call on coroutine task";
    return NULL;
  }

  message->SetRemoteHost(server_info_.host);
  message->SetWorkerCtx(base::MessageLoop::Current(), co_resumer);

  auto channels = std::atomic_load(&roundrobin_channes_);

  if (channels->empty()) { //avoid x/0 Error
    LOG(ERROR) << " No Connection Established To Server:" << address_.IpPort();
    return NULL;
  }
  uint32_t client_index = router_counter_.fetch_add(1) % channels->size();

  RefClientChannel& client = channels->at(client_index);
  base::MessageLoop* io_loop = client->IOLoop();
  CHECK(io_loop);
  bool success = io_loop->PostTask(NewClosure(std::bind(&ClientChannel::SendRequest, client, message)));
  if (!success) {
    return NULL;
  }

  co_yield;

  return message->RawResponse();
}

uint64_t Client::ClientCount() const {
  auto channels = std::atomic_load(&roundrobin_channes_);
  return channels->size();
}

std::string Client::RouterInfo() const {
  std::ostringstream oss;
  oss << " [remote:" << address_.IpPort()
      << ", clients:" << ClientCount() << "]";
  return oss.str();
}

}//end namespace net
