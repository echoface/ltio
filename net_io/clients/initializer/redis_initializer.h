#ifndef LT_NET_CLIENT_REDIS_INITIALIZER_H_
#define LT_NET_CLIENT_REDIS_INITIALIZER_H_

#include "initializer.h"

namespace lt {
namespace net {

class RedisInitializer : public Initializer {
public:
  virtual ~RedisInitializer();
  RedisInitializer(Provider* p) :Initializer(p) {};

  //override from ProtoServiceDelegate
  void OnProtocolServiceReady(const RefProtoService& service) override;
  void OnProtocolServiceGone(const RefProtoService& service) override;
  void OnProtocolMessage(const RefProtocolMessage& message) override;
};

}  // namespace net
}  // namespace lt
#endif

