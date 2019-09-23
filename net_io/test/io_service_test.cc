#include "glog/logging.h"
#include "tcp_channel.h"
#include "io_service.h"

#include "protocol/proto_service.h"
#include "protocol/proto_service_factory.h"
#include "base/message_loop/linux_signal.h"

namespace lt {
namespace net {

class TcpProtoService : public ProtoService {
public:
  TcpProtoService() :
    ProtoService() {
  }
  ~TcpProtoService() {
  }
  bool SendRequestMessage(const RefProtocolMessage &message) override {
    return true;
  };
  bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) override {
    return true;
  };

  void OnStatusChanged(const SocketChannel* channel) override {
    LOG(INFO) << "RefTcpChannel status changed:" << channel->StatusAsString();
  }
  void OnDataFinishSend(const SocketChannel* channel) override {
    LOG(INFO) << " RefTcpChannel  data write finished";
  }

  void OnDataReceived(const SocketChannel* channel, IOBuffer *buffer) override {
    LOG(INFO) << " RefTcpChannel  recieve data";
    CHECK(channel == Channel());
    int32_t size = Channel()->Send(buffer->GetRead(), buffer->CanReadSize());
    if (size > 0) {
      buffer->Consume(size);
    }
    //channel->ShutdownChannel();
  }
};

class Srv : public IOServiceDelegate {
public:
  Srv() {
    tcp_protoservice_.reset(new TcpProtoService);
    InitWorkLoop();

    net::SocketAddr addr("127.0.0.1", 5005);
    ioservice_.reset(new IOService(addr, "tcp", &acceptor_loop_, this));
  }
  ~Srv() {
  }


  void InitWorkLoop() {
    iowork_loop_.SetLoopName("iowork_loop");
    iowork_loop_.Start();
    acceptor_loop_.SetLoopName("acceptor_loop");
    acceptor_loop_.Start();
  }

  void Start() {
    RegisterExitSignal();

    ProtoServiceFactory::Instance().RegisterCreator("tcp", []()->ProtoServicePtr {
      return ProtoServicePtr(new TcpProtoService);
    });

    ioservice_->StartIOService();

    acceptor_loop_.WaitLoopEnd();
  }

  void IOServiceStoped(IOService* ioservice) {
    LOG(INFO) << " IOService stop ok";
    CHECK(ioservice_.get() == ioservice);
    ioservice_.reset();
    acceptor_loop_.QuitLoop();
  };

  base::MessageLoop* GetNextIOWorkLoop() {
    return &iowork_loop_;
  }
  bool IncreaseChannelCount() {
    LOG(INFO) << " a new channel comming";
    return true;
  }
  void DecreaseChannelCount() {
    LOG(INFO) << " a channel going gone";
  }
  bool CanCreateNewChannel() {
    return true;
  }
  ProtoService* GetProtocolService(const std::string protocol) {
    return tcp_protoservice_.get();
  }
  void OnRequestMessage(const RefProtocolMessage& request) {

  };
private:
  void RegisterExitSignal() {
    //CHECK(acceptor_loop_.IsInLoopThread());
    base::Signal::signal(10, std::bind(&Srv::ExitSignalHandle, this));
  }
  void ExitSignalHandle() {
    LOG(INFO) << " ExitSignalHandle quit AcceptorLoop";
    acceptor_loop_.PostTask(
      NewClosure(std::bind(&IOService::StopIOService, ioservice_.get())));
  }
  base::MessageLoop iowork_loop_;
  base::MessageLoop acceptor_loop_;

  IOServicePtr ioservice_;
  ProtoServicePtr tcp_protoservice_;
};

}}

using namespace lt;

int main(int argc, char** argv) {
  net::Srv s;
  s.Start();
  //LOG(INFO) << " main is going to end";
  return 0;
}
