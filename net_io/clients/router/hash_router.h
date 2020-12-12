#ifndef LT_NET_HASH_CLIENT_ROUTER_H_H
#define LT_NET_HASH_CLIENT_ROUTER_H_H

#include <memory>
#include <vector>
#include "client_router.h"
#include <net_io/clients/client.h>
#include <net_io/codec/codec_message.h>

namespace lt {
namespace net {

template<typename Hasher>
class HashRouter : public ClientRouter {
public:
  HashRouter() {};
  virtual ~HashRouter() {};

  void AddClient(RefClient&& client) override {
    clients_.push_back(std::move(client));
  };

  RefClient GetNextClient(const std::string& hash_key,
                          CodecMessage* hint_message = NULL) override {

    uint64_t value = hasher_(hash_key);
    RefClient client = clients_[value % clients_.size()];
    return client;
  };

private:
  Hasher hasher_;
  std::vector<RefClient> clients_;
};

}} //end lt::net

#endif
