#include "maglev_router.h"


namespace net {

MaglevRouter::MaglevRouter() {}
MaglevRouter::~MaglevRouter() {}

void MaglevRouter::StopAllClients() {
}

void MaglevRouter::AddClient(ClientPtr&& client) {

}

Client* MaglevRouter::GetNextClient(const std::string& key, ProtocolMessage* request) {
  return clients_.back().get();
};


} // namespace katran
