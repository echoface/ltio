#include <string>
#include <inttypes.h>
#include <sys/socket.h>

#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "glog/logging.h"
#include "url_string_utils.h"
#include "utils/string/str_utils.hpp"

namespace net {
namespace url {

static std::string ip_port(":");
static std::string scheme_host("://");

/* only for scheme://xx.xx.xx.xx:port format */
/* case 1: http://127.0.0.1:80 */
/* case 2: ://127.0.0.1:80 */
/* case 3: ://127.0.0.1: */
/* case 4: ://ui.jd.cn:*/
/* case 5: ://ui.jd.cn*/

bool ParseURI(const std::string uri, SchemeIpPort& out) {

  out.port = 0;

  auto result = base::StrUtils::Split(uri, scheme_host, false);
  std::cout << " result:" << base::StrUtils::Join(result, std::string("==")) << " size:" <<  result.size() << std::endl;
  if (result.size() != 2) {
    return false;
  }
  out.protocol = result[0].empty() ?  "http" : result[0];

  auto host_port = base::StrUtils::Split(result[1], ip_port);
  switch (host_port.size()) {
    case 1:
      out.host = host_port[0];
      out.port = 0;
      break;
    case 2:
      out.host = host_port[0];
      out.port = base::StrUtils::Parse<uint16_t>(host_port[1]);
      break;
    default:
      return false;
  }

  if (!out.host.empty()) {
    return HostResolve(out.host, out.host_ip);
  }
  return true;
}

bool HostResolve(const std::string& host, std::string& host_ip) {

  int status;
  struct addrinfo hints, *p = NULL;
  struct addrinfo *serv_info = NULL;

  memset(&hints, 0, sizeof hints);

  hints.ai_family   = AF_UNSPEC;
  hints.ai_flags    = AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(host.c_str(), NULL, &hints, &serv_info);
  if (status == -1) {
    return false;
  }

  for (p = serv_info; p != NULL; p = p->ai_next) {
    struct in_addr  *addr;
    switch(p->ai_family) {
      case AF_INET: {
        struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv->sin_addr);

        std::vector<char> ip_str(INET_ADDRSTRLEN);
        if (inet_ntop(p->ai_family, addr, &ip_str[0], ip_str.size())) {
          host_ip.assign((const char*)&ip_str[0], ip_str.size());
        }
      } break;
      case AF_INET6: {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = (struct in_addr *) &(ipv6->sin6_addr);

        std::vector<char> ip_str(INET6_ADDRSTRLEN);
        if (inet_ntop(AF_INET6, addr, &ip_str[0], ip_str.size())) {
          host_ip.assign((const char*)&ip_str[0], ip_str.size());
        }
      } break;
      default:{
      }break;
    }
  }
  freeaddrinfo(serv_info);
  return !host_ip.empty();
}

}}
