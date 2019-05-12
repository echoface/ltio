#ifndef LT_NET_CLIENT_ROUTER_H_H
#define LT_NET_CLIENT_ROUTER_H_H

#include <memory>
#include <vector>
#include <atomic>
#include <clients/client.h>
#include <protocol/proto_message.h>

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
  virtual Client* GetNextClient(ProtocolMessage* request) = 0;
protected:
};

class RoundRobinRouter : public ClientRouter {
  public:
    RoundRobinRouter() {};
    ~RoundRobinRouter() {};

    void StopAllClients() override;
    void AddClient(ClientPtr&& client) override;
    Client* GetNextClient(ProtocolMessage* request) override;
  private:
    std::vector<ClientPtr> clients_;  
    std::atomic_uint32_t round_index_;
};

}
#endif
