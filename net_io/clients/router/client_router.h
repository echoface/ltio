#ifndef LT_NET_CLIENT_ROUTER_H_H
#define LT_NET_CLIENT_ROUTER_H_H

#include <memory>
#include <vector>
#include <atomic>
#include <net_io/clients/client.h>
#include <net_io/protocol/proto_message.h>


namespace lt {
namespace net {

typedef std::unique_ptr<Client> ClientPtr;
/**
 * ClientRouter manager a group of client(a client has N connection)
 * Decide the client choose strategy
 * */
class ClientRouter {
public:
  ClientRouter() {};
  virtual ~ClientRouter() {};

  virtual void StopAllClients() = 0;

  virtual void AddClient(ClientPtr&& client) = 0;
  virtual Client* GetNextClient(const std::string& key,
                                ProtocolMessage* request = NULL) = 0;
};

}}//lt::net
#endif
