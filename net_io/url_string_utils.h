#ifndef NET_URL_STRING_HELPER_H_H
#define NET_URL_STRING_HELPER_H_H

#include <string>
#include <inttypes.h>
#include <unordered_map>

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

typedef std::unordered_map<std::string, std::string> QueryMap;
typedef struct {
  uint16_t port;
  std::string user;
  std::string passwd;
  std::string host;
  std::string host_ip;
  std::string protocol;
  QueryMap querys;
  void reset() {
    port = 0;
    user.clear();
    passwd.clear();
    host.clear();
    host_ip.clear();
    protocol.clear();
    querys.clear();
  }
} RemoteInfo;
/* uniform uri parse: http://user:password@host:port?arg1=1&arg2=2
 * protocol://user:password@hots:port?query_string*/
bool ParseRemote(const std::string& in, RemoteInfo& out, bool resolve = true);

}}
#endif
