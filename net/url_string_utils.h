#ifndef NET_URL_STRING_HELPER_H_H
#define NET_URL_STRING_HELPER_H_H

#include <string>
#include <inttypes.h>

namespace net {

namespace url {

typedef struct {
  uint16_t port;
  std::string host;
  std::string host_ip;
  std::string protocol;
} SchemeIpPort;

/* only for scheme://xx.xx.xx.xx:port format */
bool ParseURI(const std::string, SchemeIpPort& out);
/* host resolve */
bool HostResolve(const std::string& host, std::string& host_ip);


}}
#endif
