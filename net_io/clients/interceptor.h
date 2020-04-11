#ifndef _LT_NET_CLIENT_INTERCEPTOR_H_H
#define _LT_NET_CLIENT_INTERCEPTOR_H_H

#include <initializer_list>

namespace lt {
namespace net {

class Client;
class CodecMessage;

class Interceptor {
public:
  Interceptor() {};
  virtual ~Interceptor() {};
  virtual CodecMessage*
    Intercept(const Client* c, const CodecMessage* req) = 0;
};
typedef std::initializer_list<Interceptor*> InterceptArgList;


}} //end lt::net
#endif
