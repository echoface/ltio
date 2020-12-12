#include "maglev_router.h"

#include "thirdparty/murmurhash/MurmurHash3.h"

namespace lt {
namespace net {

static uint32_t k_num_seed = 0x12345678;
static uint32_t k_hash_seed = 0x87654321;

void MaglevRouter::AddClient(RefClient&& client) {
  clients_.push_back(client);
}

void MaglevRouter::StartRouter() {
  lb::EndpointList node_list;
  for (uint32_t i = 0; i < clients_.size(); i++) {
    uint32_t hash_value = 0;
    const std::string hash_key = clients_[i]->RemoteIpPort();
    MurmurHash3_x86_32(hash_key.data(), hash_key.size(), k_hash_seed, &hash_value); 

    node_list.push_back({i, uint32_t(clients_.size()), hash_value});
  }
  lookup_table_ = lb::MaglevV2::GenerateHashRing(node_list);
}

RefClient MaglevRouter::GetNextClient(const std::string& key,
                                      CodecMessage* request) {
  if (clients_.empty()) return nullptr; 

  uint32_t hash_value = 0;
  MurmurHash3_x86_32(key.data(), key.size(), k_num_seed, &hash_value); 
  uint32_t idx = lookup_table_[hash_value % lookup_table_.size()];

  return idx < clients_.size() ? clients_[idx] : nullptr;
}

}} // namespace lt::net
