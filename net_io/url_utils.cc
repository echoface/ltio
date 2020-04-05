#include <string>
#include <inttypes.h>
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
#include <base/string/string_view.hpp>
#include <base/utils/string/str_utils.h>

#include "url_utils.h"

namespace lt {
namespace net {
namespace url {

static std::string ip_port(":");
static std::string scheme_host("://");

/* full format [scheme://xx.xx.xx.xx:port] */

/* case 1: http://127.0.0.1:80 */
/* case 2: ://127.0.0.1:80 */
/* case 3: ://127.0.0.1: */
/* case 4: ://ui.jd.cn: */
/* case 5: ://ui.jd.cn */

/* default protocol: http, default port: 80  */
bool ParseURI(const std::string uri, SchemeIpPort& out) {

  out.port = 0;
  out.protocol = "http";

  nonstd::string_view in(uri);

  std::string::size_type find_start = 0;
  std::string::size_type pos = in.find("://", find_start);
  if (pos != std::string::npos) {
    out.protocol = in.substr(find_start, pos).to_string();
    find_start = pos + sizeof("://") - 1;
  }

  pos = in.find(":", find_start);
  if (pos != std::string::npos) {
    uint32_t len = pos - find_start;

    base::StrUtil::ParseTo(in.substr(pos + 1).to_string(), out.port);
    out.host = in.substr(find_start, len).to_string();
    find_start = pos + sizeof(":") - 1;
  } else {
    out.host = in.substr(find_start).to_string();
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
    LOG(ERROR) << "getaddrinfo failed, host:" << host.c_str();
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

// use c++17 std::string_view avoid memory allocator
/* protocol://user:password@host:port?query_string*/
bool ParseRemote(const std::string& str, RemoteInfo& out, bool resolve) {

  nonstd::string_view in = nonstd::string_view(str);

  std::string::size_type find_start = 0;
  std::string::size_type pos = in.find("://", find_start);
  if (pos != std::string::npos) {
    out.protocol = in.substr(find_start, pos).to_string();
    find_start = pos + sizeof("://") - 1;
  }

  pos = in.find("@", find_start);
  if (pos != std::string::npos) {//got user:psd
    uint32_t len = pos - find_start;
    auto user_psd = base::StrUtil::Split(in.substr(find_start, len).to_string(), ":", false);
    out.user = user_psd[0];
    if (user_psd.size() > 1) {
      out.passwd = user_psd[1];
    }
    find_start = pos + sizeof("@") - 1;
  }

  pos = in.find("?", find_start);
  {
    uint32_t len = pos - find_start;
    auto host_port = base::StrUtil::Split(in.substr(find_start, len).to_string(), ":", false);
    out.host = host_port.front();
    if (host_port.size() > 1) {
      out.port = base::StrUtil::Parse<uint16_t>(host_port[1]);
    }
    if (resolve) {
      HostResolve(out.host, out.host_ip);
    }
    if (pos == std::string::npos) {
      return true;
    }

    find_start = pos + sizeof("?") - 1;
  }

  auto querys = base::StrUtil::Split(in.substr(find_start).to_string(), '&');
  for (auto& query : querys) {
    auto kv = base::StrUtil::Split(query, "=", false);
    if (kv.size() != 2) {
      continue;
    }
    out.querys.insert(std::make_pair(kv.front(), kv.back()));
  }
  return true;
}

std::string RemoteInfo::HostIpPort() const {
  return base::StrUtil::Concat(host, "[", host_ip, "]:", port);
}

}}} //net::url
