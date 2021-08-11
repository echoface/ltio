#include "base/coroutine/co_runner.h"
#include "base/message_loop/message_loop.h"
#include "net_io/clients/client.h"
#include "net_io/co_so/io_service.h"

using lt::net::RefCodecMessage;

class H : public lt::net::CodecService::Handler {
public:
  void OnCodecMessage(const RefCodecMessage& message) override {
    LOG(INFO) << __FUNCTION__ << " handler enter";
    LOG(INFO) << "message:" << message->Dump();
    auto codec = message->GetIOCtx().codec.lock();
    ignore_result(codec->SendResponse(message.get(), message.get()));
  };

};

int main() {
  FLAGS_v = 26;

  std::cout <<
    "usage: telnet localhost 5006\n"
    "then press ctrl-] to ctrl mode\n"
    ">set crlf\n" << std::endl;

  base::MessageLoop loop("io");
  loop.Start();

  co_go &loop << [&]() {
    IPEndPoint address("127.0.0.1", 5006);
    coso::IOService ioservice(address);
    ioservice.WithProtocal("line");
    ioservice.WithHandler(new H());
    ioservice.Run();
  };
  loop.WaitLoopEnd();
}
