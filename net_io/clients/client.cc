#include "client.h"

#include "base/base_constants.h"
#include "base/utils/string/str_utils.h"
#include "base/coroutine/coroutine_runner.h"
#include "glog/logging.h"

namespace lt {
namespace net {

Client::Client(base::MessageLoop* loop, const url::RemoteInfo& info)
  : address_(info.host_ip, info.port),
    remote_info_(info),
    work_loop_(loop),
    stopping_(false),
    delegate_(NULL) {

  next_index_ = 0;
  CHECK(work_loop_);
  connector_ = std::make_shared<Connector>(work_loop_, this);
  auto empty_list = std::make_shared<ClientChannelList>();
  std::atomic_store(&in_use_channels_, empty_list);
}

Client::~Client() {
  Finalize();
}

void Client::SetDelegate(ClientDelegate* delegate) {
  delegate_ = delegate;
}

void Client::Initialize(const ClientConfig& config) {
  config_ = config;
  for (uint32_t i = 0; i < config_.connections; i++) {
    auto functor = std::bind(&Connector::Launch, connector_, address_);
    work_loop_->PostTask(NewClosure(functor));
  }
}


void Client::Finalize() {
  if (stopping_.exchange(true)) {
    return;
  }

  if (work_loop_->IsInLoopThread()) {
    connector_->Stop(); //sync
  } else {
    work_loop_->PostTask(NewClosure(std::bind(&Connector::Stop, connector_)));
  }

  auto channels = std::atomic_load(&in_use_channels_);
  for (auto& ch : *channels) {
    ch->IOLoop()->PostTask(NewClosure(std::bind(&ClientChannel::Close, ch)));
  }
  auto empty_list = std::make_shared<ClientChannelList>();
  std::atomic_store(&in_use_channels_, empty_list);
}

void Client::OnClientConnectFailed() {
  CHECK(work_loop_->IsInLoopThread());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " connect failed";
  if (stopping_ || config_.connections <= ConnectedCount()) {
    return;
  }
  auto functor = std::bind(&Connector::Launch, connector_, address_);
  work_loop_->PostDelayTask(NewClosure(functor), config_.recon_interval);
  VLOG(GLOG_VERROR) << __FUNCTION__ << ClientInfo()
    << " try after " << config_.recon_interval << " ms";
}

base::MessageLoop* Client::next_client_io_loop() {
  base::MessageLoop* io_loop = NULL;
  if (delegate_) {
    io_loop = delegate_->NextIOLoopForClient();
  }
  return io_loop ? io_loop : work_loop_;
}

RefClientChannel Client::get_ready_channel() {
  RefClientChannelList channels = std::atomic_load(&in_use_channels_);
  if (!channels || channels->size() == 0) return NULL;

  int32_t max_step = std::min(10, int(channels->size()));
  do {
    uint32_t idx = next_index_.fetch_add(1) % channels->size();
    auto& ch = channels->at(idx);
    if (ch->Ready()) {
      return ch;
    }
  } while(max_step--);

  return NULL;
}

void Client::OnNewClientConnected(int socket_fd, SocketAddr& local, SocketAddr& remote) {
  CHECK(work_loop_->IsInLoopThread());

  base::MessageLoop* io_loop = next_client_io_loop();

  auto proto_service =
    ProtoServiceFactory::NewClientService(remote_info_.protocol, io_loop);
  proto_service->BindToSocket(socket_fd, local, remote);

  auto client_channel = CreateClientChannel(this, proto_service);
  client_channel->SetRequestTimeout(config_.message_timeout);

  in_use_channels_->push_back(client_channel);

  //这里的交互界面是Connector 《== 》与ClientChannel
  if (io_loop->IsInLoopThread()) {
    client_channel->StartClientChannel();
  } else {
    io_loop->PostTask(NewClosure(
        std::bind(&ClientChannel::StartClientChannel, client_channel)));
  }
  VLOG(GLOG_VINFO) << __FUNCTION__ << ClientInfo() << " connected, initializing...";
}

void Client::OnClientChannelInited(const ClientChannel* channel) {;
  if (!work_loop_->IsInLoopThread()) {
    auto functor = std::bind(&Client::OnClientChannelInited, this, channel);
    work_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter, in worker loop(connector loop)";
  if (delegate_) {
    delegate_->OnClientChannelReady(channel);
  }
  VLOG(GLOG_VINFO) << __FUNCTION__ << ClientInfo() << " new channel ready for use";
}

/*on the loop of client IO, need managed by connector loop*/
void Client::OnClientChannelClosed(const RefClientChannel& channel) {
  if (!work_loop_->IsInLoopThread() && !stopping_) {
    auto functor = std::bind(&Client::OnClientChannelClosed, this, channel);
    work_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter in work loop";

  do {
    RefClientChannelList in_use_list = std::atomic_load(&in_use_channels_);
    RefClientChannelList new_list(new ClientChannelList(*in_use_list));

    auto iter = std::find(new_list->begin(), new_list->end(), channel);
    if (iter != new_list->end()) {
      new_list->erase(iter);
      std::atomic_store(&in_use_channels_, new_list);
    }
  }while(0);

  uint32_t connected_count = ConnectedCount();
  if (stopping_ && connected_count == 0) {
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " all client channel closed, stoping done";
    return;
  }

  if (connected_count == 0 && delegate_) {
    delegate_->OnAllClientPassiveBroken(this);
  }
  //reconnect
  if (!stopping_ && connected_count < config_.connections) {
    auto f = std::bind(&Connector::Launch, connector_, address_);
    work_loop_->PostDelayTask(NewClosure(f), config_.recon_interval);
    VLOG(GLOG_VTRACE) << __FUNCTION__ << ClientInfo() << " reconnect after:" << config_.recon_interval;
  }
  VLOG(GLOG_VINFO) << __FUNCTION__ << ClientInfo() << " a client connection gone";
}

void Client::OnRequestGetResponse(const RefProtocolMessage& request,
                                  const RefProtocolMessage& response) {
  request->SetResponse(response);
  request->GetWorkCtx().resumer_fn();
}

bool Client::AsyncSendRequest(RefProtocolMessage& req, AsyncCallBack callback) {
  base::MessageLoop* worker = base::MessageLoop::Current();
  if (!worker) {
    LOG(ERROR) << __FUNCTION__ << " Only Work IN MessageLoop";
    return false;
  }

  //IMPORTANT: avoid self holder for capture list
  ProtocolMessage* request = req.get();
  base::StlClosure resumer = [=]() {
    if (worker->IsInLoopThread()) {
      callback(request->RawResponse());
    } else {
      auto responser = std::bind(callback, request->RawResponse());
      worker->PostTask(NewClosure(responser));
    }
  };

  req->SetWorkerCtx(worker, std::move(resumer));
  req->SetRemoteHost(remote_info_.host);

  RefClientChannel client = get_ready_channel();
  if (!client) {
    return false;
  }

  base::MessageLoop* io = client->IOLoop();
  return io->PostTask(
    NewClosure(std::bind(&ClientChannel::SendRequest, client, req)));
}

ProtocolMessage* Client::SendClientRequest(RefProtocolMessage& message) {
  if (!base::MessageLoop::Current() || !base::CoroRunner::CanYield()) {
    LOG(ERROR) << __FUNCTION__ << " must call on coroutine task";
    return NULL;
  }
  message->SetRemoteHost(remote_info_.host);
  message->SetWorkerCtx(base::MessageLoop::Current(), co_resumer);

  auto channel = get_ready_channel();
  if (!channel) {
    message->SetFailCode(MessageCode::kNotConnected);
    LOG_EVERY_N(INFO, 1000) << "no established client can use";
    return NULL;
  }

  base::MessageLoop* io_loop = channel->IOLoop();
  bool ok = io_loop->PostTask(NewClosure(std::bind(&ClientChannel::SendRequest, channel, message)));
  if (!ok) {
    message->SetFailCode(MessageCode::kConnBroken);
    LOG(ERROR) << "schedule task to io_loop failed";
    return NULL;
  }

  co_yield;

  return message->RawResponse();
}

uint64_t Client::ConnectedCount() const {
  auto channels = std::atomic_load(&in_use_channels_);
  return channels->size();
}

std::string Client::ClientInfo() const {
  std::ostringstream oss;
  oss << "[remote:" << RemoteIpPort()
      << ", connections:" << ConnectedCount()
      << "]";
  return oss.str();
}

std::string Client::RemoteIpPort() const {
  return base::StrUtil::Concat(remote_info_.host_ip, ":", remote_info_.port);
}

}}//end namespace lt::net
