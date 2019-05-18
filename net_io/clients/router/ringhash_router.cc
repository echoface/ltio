#include "ringhash_router.h"

namespace net {

void RingHashRouter::StopAllClients() {
  ;
}

void RingHashRouter::AddClient(ClientPtr&& client) {

}

Client* RingHashRouter::GetNextClient(const std::string& key,
                                      ProtocolMessage* request) {

}

}//end net
