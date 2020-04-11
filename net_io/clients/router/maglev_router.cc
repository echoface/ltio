#include "maglev_router.h"

#include "thirdparty/murmurhash/MurmurHash3.h"

namespace lt {
namespace net {

static uint32_t k_num_seed = 0x12345678;
static uint32_t k_hash_seed = 0x87654321;

MaglevClientEndpoint::MaglevClientEndpoint(RefClient& client)
  : client_(client){

  const std::string hash_key = client->RemoteIpPort();
  MurmurHash3_x86_32(hash_key.data(), hash_key.size(), k_num_seed, &node_.num); 
  MurmurHash3_x86_32(hash_key.data(), hash_key.size(), k_hash_seed, &node_.hash); 
}

void MaglevRouter::AddClient(RefClient&& client) {
  MaglevClientEndpoint ep(client);
  auto result = nodes_.insert(std::make_pair(ep.IdentifyNum(), ep)); 
  CHECK(result.second);
}

void MaglevRouter::StartRouter() {
  MaglevHelper::NodeList node_list;
  for (auto& kv : nodes_) {
    kv.second.SetNodeWeight(nodes_.size());
    node_list.push_back(kv.second.Node());
  }
  lookup_table_ = MaglevHelper::GenerateMaglevHash(node_list);
}

RefClient MaglevRouter::GetNextClient(const std::string& hash_key,
                                      CodecMessage* request) {
  if (nodes_.empty()) {
    return nullptr;
  }

  uint32_t hash_value = 0;
  MurmurHash3_x86_32(hash_key.data(), hash_key.size(), k_num_seed, &hash_value); 
  uint32_t num = lookup_table_[hash_value % lookup_table_.size()];
  const auto iter = nodes_.find(num);
  if (iter != nodes_.end()) {
    return iter->second.client_; 
  }
  LOG(ERROR) << __FUNCTION__ << " should not rearch here";
  return nullptr;
}

}} // namespace lt::net
