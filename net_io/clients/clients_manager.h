#ifndef _NET_CLIENTS_MANAGER_H_H
#define _NET_CLIENTS_MANAGER_H_H

#include "client.h"

namespace lt {
namespace net {


class ClientsManager {
public:
  ClientsManager();
  ~ClientsManager();

  void RegisterClient();
private:

};

}} //end net
#endif
