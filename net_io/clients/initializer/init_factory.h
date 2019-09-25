#ifndef NET_CLIENT_INITIALIZER_FACTORY_H
#define NET_CLIENT_INITIALIZER_FACTORY_H

#include <memory>
#include <functional>
#include <unordered_map>
#include "initializer.h"
#include "net_callback.h"

namespace lt {
namespace net {

typedef std::function<Initializer* (Initializer::Provider* p)> InitializerCreator;

class ClientInitialzerFactory {
public:
  static ClientInitialzerFactory& Instance();
  static Initializer* Create(const std::string& proto, Initializer::Provider* p);
  // not thread safe,
  void RegisterCreator(const std::string, InitializerCreator);

  ClientInitialzerFactory();
private:
  void InitInnerDefault();
  Initializer* create_internal(const std::string& proto, Initializer::Provider* p);
  std::unordered_map<std::string, InitializerCreator> creators_;
};

}}
#endif

