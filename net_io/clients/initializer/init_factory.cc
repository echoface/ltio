#include "init_factory.h"
#include "memory/lazy_instance.h"
#include "redis_initializer.h"


namespace lt {
namespace net {

//static
ClientInitialzerFactory& ClientInitialzerFactory::Instance() {
  static base::LazyInstance<ClientInitialzerFactory> instance = LAZY_INSTANCE_INIT;
  return instance.get();
}

ClientInitialzerFactory::ClientInitialzerFactory() {
  InitInnerDefault();
}

void ClientInitialzerFactory::InitInnerDefault() {
  creators_.insert(std::make_pair("redis", [](Initializer::Provider* p)->Initializer* {
    return new RedisInitializer(p);
  }));
}


Initializer* ClientInitialzerFactory::Create(const std::string& proto,
                                             Initializer::Provider* p) {
  return Instance().create_internal(proto, p);
}

Initializer* ClientInitialzerFactory::create_internal(const std::string& proto,
                                                      Initializer::Provider* p) {
  auto iter = creators_.find(proto);
  if (iter == creators_.end()) {
    return new Initializer(p);
  }
  return iter->second(p);
}

void ClientInitialzerFactory::RegisterCreator(const std::string proto,
                                              InitializerCreator creator) {
  creators_[proto] = creator;
}

}}
