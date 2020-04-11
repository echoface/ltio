
#include "roundrobin_router.h"

namespace lt {
namespace net {

void RoundRobinRouter::AddClient(RefClient&& client) {
  clients_.push_back(std::move(client));
}

RefClient RoundRobinRouter::GetNextClient(const std::string& key,
                                          CodecMessage* request) {
  uint32_t idx = round_index_++ % clients_.size();
  return clients_[idx];
}

}}//end lt::net
