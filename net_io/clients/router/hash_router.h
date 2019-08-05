#ifndef LT_NET_HASH_CLIENT_ROUTER_H_H
#define LT_NET_HASH_CLIENT_ROUTER_H_H

#include <memory>
#include <vector>
#include <clients/client.h>
#include <protocol/proto_message.h>
#include "client_router.h"

namespace lt {
namespace net {

template<typename Hasher>
class HashRouter : public ClientRouter {
public:
  HashRouter() {};
  virtual ~HashRouter() {};

  void StopAllClients() override {
    for (auto& client : clients_) {
      client->FinalizeSync();
    }
  }

  void AddClient(ClientPtr&& client) override {
    LOG(INFO) << __FUNCTION__ << " c:" << client.get();
    clients_.push_back(std::move(client));
  }

  Client* GetNextClient(const std::string& key,
                        ProtocolMessage* request) override {
    uint64_t value = hasher_(key);
    LOG(INFO) << __FUNCTION__ << " value:" << value << " idx:" << value % clients_.size();
    return clients_[value % clients_.size()].get();
  }
private:
  Hasher hasher_;
  std::vector<ClientPtr> clients_;
};

}} //end lt::net

#endif
