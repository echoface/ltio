#include "ringhash_router.h"

namespace lt {
namespace net {

RingHashRouter::RingHashRouter(uint32_t vnode_count)
  : vnode_count_(vnode_count) {
}

void RingHashRouter::StopAllClients() {
  for (auto& client : all_clients_) {
    client->Finalize();
  }
}

void RingHashRouter::AddClient(ClientPtr&& client) {
  for (uint32_t i = 0; i < vnode_count_; i++) {
    ClientNode node(client.get(), i);
    clients_.insert(node);
  }
  all_clients_.push_back(std::move(client));
}

Client* RingHashRouter::GetNextClient(const std::string& key, ProtocolMessage* request) {
  //uint32_t hash_value = ::crc32(0x80000000, (const unsigned char*)key.data(), key.size());
  uint32_t hash_value = 0;
  MurmurHash3_x86_32(key.data(),
                     key.size(),
                     0x80000000,
                     &hash_value);
  NodeContainer::iterator iter = clients_.find(hash_value);
  if (iter == clients_.end()) {
    return NULL;
  }
  return iter->second.client;
}

}}//end net
