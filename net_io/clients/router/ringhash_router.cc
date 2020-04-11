#include "ringhash_router.h"

namespace lt {
namespace net {

RingHashRouter::RingHashRouter(uint32_t vnode_count)
  : vnode_count_(vnode_count) {
}

void RingHashRouter::AddClient(RefClient&& client) {
  for (uint32_t i = 0; i < vnode_count_; i++) {
    clients_.insert(ClientNode(client, i));
  }
  all_clients_.push_back(std::move(client));
}

RefClient RingHashRouter::GetNextClient(const std::string& key, CodecMessage* request) {
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
