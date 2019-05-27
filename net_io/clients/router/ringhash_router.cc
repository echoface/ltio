#include "ringhash_router.h"

namespace net {

RingHashRouter::RingHashRouter(uint32_t vnode_count)
  : vnode_count_(vnode_count) {
}

void RingHashRouter::StopAllClients() {
  for (auto& client : all_clients_) {
    client->FinalizeSync();
  }
}

void RingHashRouter::AddClient(ClientPtr&& client) {
  for (uint32_t i = 0; i < vnode_count_; i++) {
    ClientNode node(client.get(), i);
    clients_.insert(node);
  }
  LOG(INFO) << "ringhash router add client:" << client->RemoteIpPort();
  all_clients_.push_back(std::move(client));
}

Client* RingHashRouter::GetNextClient(const std::string& key, ProtocolMessage* request) {
  //uint32_t hash_value = ::crc32(0x80000000, (const unsigned char*)key.data(), key.size());
  uint32_t hash_value = 0;
  MurmurHash3_x86_32(key.data(),
                     key.size(),
                     0x80000000,
                     &hash_value);
  CHashMap::iterator iter = clients_.find(hash_value);
  if (iter == clients_.end()) {
    return NULL;
  }
  return iter->second.client;
}

}//end net
