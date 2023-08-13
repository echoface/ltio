#ifndef LT_NET_HANDLER_FACTORY_H_H
#define LT_NET_HANDLER_FACTORY_H_H

#include <chrono>  // std::chrono::seconds
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "base/lt_macro.h"
#include "base/coroutine/co_runner.h"
#include "net_io/io_service.h"

namespace lt {
namespace net {

template <typename Functor, typename Context, bool coro = true>
class HandlerFactory : public CodecService::Handler {
public:
  HandlerFactory(const Functor& handler) : handler_(handler) {}

  void OnCodecMessage(const RefCodecMessage& message) override {
    if (coro) {
      co_go std::bind(handler_, Context::New(message));
      return;
    }
    handler_(Context::New(message));
  };

private:
  Functor handler_;
};

}  // namespace net
}  // namespace lt

template <typename Functor, typename Context>
lt::net::CodecService::Handler* NewHandler(const Functor& func) {
  return new lt::net::HandlerFactory<Functor, Context, false>(func);
}

template <typename Functor, typename Context>
lt::net::CodecService::Handler* NewCoroHandler(const Functor& func) {
  return new lt::net::HandlerFactory<Functor, Context>(func);
}


#endif
