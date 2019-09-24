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
    typedef std::function<void(RefProtoService&)> InitCallback;

    virtual ~Initializer();
    Initializer(const url::RemoteInfo& remote, const ClientConfig& conf);
    void SetSuccessCallback(InitCallback fn) {success_fn_ = fn;}

    virtual void Init(RefProtoService& service);

    //override from ProtoServiceDelegate
    void OnProtocolServiceReady(const RefProtoService& service) override;
    void OnProtocolServiceGone(const RefProtoService& service) override;
    void OnProtocolMessage(const RefProtocolMessage& message) override;
  protected:
    InitCallback success_fn_;
    const url::RemoteInfo& remote_;
    const ClientConfig& config_;
    std::set<RefProtoService> client_serivces_;
};

}  // namespace net
}  // namespace lt
#endif
