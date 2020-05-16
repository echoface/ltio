#ifndef _NET_SERVER_REQUEST_CONTEXT_H_
#define _NET_SERVER_REQUEST_CONTEXT_H_

#include "net_io/codec/codec_factory.h"

namespace lt {
namespace net {

class CommonRequestContext {
public:
  CommonRequestContext(const RefCodecMessage&& req);
  ~CommonRequestContext();

  template<class T>
  const T* GetRequest() const {
    return static_cast<T*>(request_.get());
  }
  void Send(const RefCodecMessage& res);
  bool IsResponded() const {return done_;}
private:
  void do_response(const RefCodecMessage&);

  bool done_ = false;
  RefCodecMessage request_;
  DISALLOW_COPY_AND_ASSIGN(CommonRequestContext);
};

}}
#endif

