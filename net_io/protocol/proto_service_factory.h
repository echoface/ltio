#ifndef NET_PROTOCOL_SERVICE_FACTORY_H
#define NET_PROTOCOL_SERVICE_FACTORY_H

#include "proto_service.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <net_io/net_callback.h>
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

typedef std::function<RefProtoService (base::MessageLoop* loop)> ProtoserviceCreator;

class ProtoServiceFactory {
public:
  static RefProtoService NewServerService(const std::string& proto,
                                          base::MessageLoop* loop);
  static RefProtoService NewClientService(const std::string& proto,
                                          base::MessageLoop* loop);

  static bool HasCreator(const std::string&);

  // not thread safe,
  // this can cover the default protoservice or add new protocol support
  static void RegisterCreator(const std::string, ProtoserviceCreator);
public:
  ProtoServiceFactory();
private:
  void InitInnerDefault();

  RefProtoService CreateProtocolService(const std::string&,
                                        base::MessageLoop*,
                                        bool server_service);

  std::unordered_map<std::string, ProtoserviceCreator> creators_;
};

}}
#endif
