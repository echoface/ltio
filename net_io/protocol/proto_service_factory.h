#ifndef NET_PROTOCOL_SERVICE_FACTORY_H
#define NET_PROTOCOL_SERVICE_FACTORY_H

#include <memory>
#include <functional>
#include <unordered_map>
#include "proto_service.h"
#include "net_callback.h"

namespace net {

typedef std::function<RefProtoService()> ProtoserviceCreator;

class ProtoServiceFactory {
public:
  static ProtoServiceFactory& Instance();
  static RefProtoService Create(const std::string& proto, bool serve_server);
  // not thread safe,
  // this can cover the default protoservice or add new protocol support
  bool HasProtoServiceCreator(const std::string&);
  void RegisterCreator(const std::string, ProtoserviceCreator);

  ProtoServiceFactory();
private:
  void InitInnerDefault();
  std::unordered_map<std::string, ProtoserviceCreator> creators_;
};

}
#endif
