
#include "proto_service_factory.h"

#include "memory/lazy_instance.h"
#include "redis/resp_service.h"
#include "raw/raw_proto_service.h"
#include "line/line_proto_service.h"
#include "http/http_proto_service.h"

namespace lt {
namespace net {

//static
ProtoServiceFactory& ProtoServiceFactory::Instance() {
  static base::LazyInstance<ProtoServiceFactory> instance = LAZY_INSTANCE_INIT;
  return instance.get();
}

ProtoServiceFactory::ProtoServiceFactory() {
  InitInnerDefault();
}

//Static
RefProtoService ProtoServiceFactory::Create(const std::string& proto, bool server_side) {
  static RefProtoService _null;

  auto& builder = Instance().creators_[proto];
  if (builder) {
    auto protocol_service = builder();
    protocol_service->SetIsServerSide(server_side);
    return protocol_service;
  }
  LOG(ERROR) << __FUNCTION__ << " Protocol:" << proto << " Not Supported";
  return _null;
}

// not thread safe,
void ProtoServiceFactory::RegisterCreator(const std::string proto,
                                          ProtoserviceCreator creator) {
  creators_[proto] = creator;
}

bool ProtoServiceFactory::HasProtoServiceCreator(const std::string& proto) {
  return creators_[proto] ? true : false;;
}

void ProtoServiceFactory::InitInnerDefault() {
  creators_.insert(std::make_pair("line", []()->RefProtoService{
    std::shared_ptr<LineProtoService> service(new LineProtoService);
    return std::static_pointer_cast<ProtoService>(service);
  }));
  creators_.insert(std::make_pair("http", []()->RefProtoService{
    std::shared_ptr<HttpProtoService> service(new HttpProtoService);
    return std::static_pointer_cast<ProtoService>(service);
  }));
  creators_.insert(std::make_pair("raw", []()->RefProtoService{
    std::shared_ptr<RawProtoService> service(new RawProtoService);
    return std::static_pointer_cast<ProtoService>(service);
  }));
  creators_.insert(std::make_pair("redis", []()->RefProtoService{
    std::shared_ptr<RespService> service(new RespService);
    return std::static_pointer_cast<ProtoService>(service);
  }));
}

}}//end namespace net
