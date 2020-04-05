#ifndef LT_NET_CLIENT_ROUTER_H_H
#define LT_NET_CLIENT_ROUTER_H_H

#include <memory>
#include <vector>
#include <atomic>
#include <net_io/clients/client.h>
#include <net_io/protocol/proto_message.h>


namespace lt {
namespace net {


REF_TYPEDEFINE(Client);
/**
 * Router:
 * ClientRouter manager a group of client(a client has N connection)
 *
 * route reqeust to diffrent client do network request
 * */
class ClientRouter {
public:
  ClientRouter() {};
  virtual ~ClientRouter() {};

  //** some of router need re-calculate values and adjustment
  //** after StartRouter, DO NOT AddClient ANY MORE
  virtual void StartRouter() {};
  virtual void AddClient(RefClient&& client) = 0;
  virtual RefClient GetNextClient(const std::string& hash_key,
                                  ProtocolMessage* hint_message = NULL) = 0;
};

}}//lt::net
#endif
