#ifndef _NET_HTTP_SERVER_CONTEXT_H_H
#define _NET_HTTP_SERVER_CONTEXT_H_H

#include "protocol/http/http_proto_service.h"

namespace lt {
namespace net {

class HttpContext {
  public:
    HttpContext(const RefHttpRequest& request);

    HttpRequest* Request() {return (HttpRequest*)request_.get();}

    void ReplyFile(const std::string&, uint16_t code = 200);

    void ReplyJson(const std::string& json, uint16_t code = 200);

    void ReplyString(const char* content, uint16_t code = 200);
    void ReplyString(const std::string& content, uint16_t code = 200);

    void DoReplyResponse(RefHttpResponse& response);

    bool DidReply() const {return did_reply_;};
  private:
    RefHttpRequest request_;
    bool did_reply_ = false;
    base::MessageLoop* io_loop_ = NULL;
};

}}
#endif
