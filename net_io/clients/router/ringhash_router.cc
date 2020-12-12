#include "ringhash_router.h"

namespace lt {
namespace net {

RingHashRouter::ClientNode::ClientNode(RefClient client, uint32_t id)
  : client(client),
    vnode_id(id),
    hash_key(client->RemoteIpPort() + std::to_string(id)) {
}

uint32_t RingHashRouter::NodeHasher::operator()(const ClientNode &node) {
  uint32_t out = 0;
  MurmurHash3_x86_32(node.hash_key.data(),
                     node.hash_key.size(),
                     0x80000000, &out);
  return out;
}

RingHashRouter::RingHashRouter(uint32_t vnode_count)
  : vnode_count_(vnode_count) {
}

void RingHashRouter::AddClient(RefClient&& client) {
  for (uint32_t i = 0; i < vnode_count_; i++) {
    clients_.insert(ClientNode(client, i));
  }
}

RefClient RingHashRouter::GetNextClient(const std::string& key, CodecMessage* request) {
  uint32_t hash_value = 0;
  MurmurHash3_x86_32(key.data(), key.size(), 0x80000000, &hash_value);

  NodeContainer::iterator iter = clients_.find(hash_value);
  if (iter == clients_.end()) {
    return NULL;
  }
  return iter->second.client;
}

}}//end lt::net
