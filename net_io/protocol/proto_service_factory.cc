#include "proto_service_factory.h"
#include "base/message_loop/message_loop.h"
#include "redis/resp_service.h"
#include "raw/raw_proto_service.h"
#include "line/line_proto_service.h"
#include "http/http_proto_service.h"

#include <base/memory/lazy_instance.h>

namespace lt {
namespace net {

//static
ProtoServiceFactory& Instance() {
  static base::LazyInstance<ProtoServiceFactory> instance = LAZY_INSTANCE_INIT;
  return instance.get();
}

ProtoServiceFactory::ProtoServiceFactory() {
  InitInnerDefault();
}

//static
RefProtoService ProtoServiceFactory::NewServerService(const std::string& proto,
                                                      base::MessageLoop* loop) {
  return Instance().CreateProtocolService(proto, loop, true);
}
//static
RefProtoService ProtoServiceFactory::NewClientService(const std::string& proto,
                                                      base::MessageLoop* loop) {
  return Instance().CreateProtocolService(proto, loop, true);
}

RefProtoService ProtoServiceFactory::CreateProtocolService(const std::string& proto,
                                                           base::MessageLoop* loop,
                                                           bool server_service) {
  auto iter = Instance().creators_.find(proto);
  if (iter != Instance().creators_.end() && iter->second) {
    auto protocol_service = iter->second(loop);
    protocol_service->SetIsServerSide(server_service);
    return protocol_service;
  }
  LOG(ERROR) << __FUNCTION__ << " Protocol:" << proto << " Not Supported";
  static RefProtoService _null;
  return _null;
}

// not thread safe,
void ProtoServiceFactory::RegisterCreator(const std::string proto,
                                          ProtoserviceCreator creator) {
  ProtoServiceFactory& factory = Instance();
  factory.creators_[proto] = creator;
}

bool ProtoServiceFactory::HasCreator(const std::string& proto) {
  ProtoServiceFactory& factory = Instance();
  return factory.creators_.find(proto) != factory.creators_.end();
}

void ProtoServiceFactory::InitInnerDefault() {
  creators_.insert(std::make_pair("line", [](base::MessageLoop* loop)->RefProtoService{
    std::shared_ptr<LineProtoService> service(new LineProtoService(loop));
    return std::static_pointer_cast<ProtoService>(service);
  }));
  creators_.insert(std::make_pair("http", [](base::MessageLoop* loop)->RefProtoService{
    std::shared_ptr<HttpProtoService> service(new HttpProtoService(loop));
    return std::static_pointer_cast<ProtoService>(service);
  }));
  creators_.insert(std::make_pair("raw", [](base::MessageLoop* loop)->RefProtoService{
    auto service = std::make_shared<RawProtoService<RawMessage>>(loop);
    return std::static_pointer_cast<ProtoService>(service);
  }));
  creators_.insert(std::make_pair("redis", [](base::MessageLoop* loop)->RefProtoService{
    std::shared_ptr<RespService> service(new RespService(loop));
    return std::static_pointer_cast<ProtoService>(service);
  }));
}

}}//end namespace net
