#ifndef LT_NET_CLIENT_INITIALIZER_H_
#define LT_NET_CLIENT_INITIALIZER_H_

#include <set>
#include <functional>
#include "url_utils.h"
#include "net_callback.h"
#include "clients/client_base.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

class Initializer : public ProtoServiceDelegate {
  public:
    typedef struct Provider {
      virtual const url::RemoteInfo& GetRemoteInfo() const = 0;
      virtual const ClientConfig& GetClientConfig() const = 0;
      virtual void OnClientServiceReady(const RefProtoService&) = 0;
    }Provider;

    virtual ~Initializer();
    Initializer(Provider* p) :provider_(p) {};

    virtual void Init(RefProtoService& service);

    //override from ProtoServiceDelegate
    void OnProtocolServiceReady(const RefProtoService& service) override;
    void OnProtocolServiceGone(const RefProtoService& service) override;
    void OnProtocolMessage(const RefProtocolMessage& message) override;
  protected:
    Provider* provider_;
    std::set<RefProtoService> client_serivces_;
};

}  // namespace net
}  // namespace lt
#endif
