#ifndef LT_CO_NET_IO_SERVICE_H_
#define LT_CO_NET_IO_SERVICE_H_

#include "net_io/common/ip_endpoint.h"
#include "net_io/codec/codec_service.h"

using lt::net::CodecService;
using lt::net::IPEndPoint;

namespace coso {

class IOService : public CodecService::Delegate {
public:
  using Handler = CodecService::Handler;

  IOService(const IPEndPoint& addr);

  IOService& WithHandler(Handler* h) {
    handler_ = h;
    return *this;
  }
  IOService& WithProtocal(const std::string& codec) {
    codec_ = codec;
    return *this;
  }

  const std::string Name() const { return address_.ToString(); }

  void Run();

  void OnCodecReady(const lt::net::RefCodecService& service) override;

  void OnCodecClosed(const lt::net::RefCodecService& service) override;

private:
  void accept_loop(int socket);
  void handle_connection(int client_socket, const IPEndPoint& peer);

  Handler* handler_;
  std::string codec_;
  IPEndPoint address_;
};

}  // namespace coso

#endif
