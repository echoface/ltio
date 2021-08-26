#ifndef _LT_NET_HTTP2_CODEC_H_
#define _LT_NET_HTTP2_CODEC_H_

#include "nghttp2/nghttp2.h"

#include "base/queue/linked_list.h"
#include "net_io/codec/codec_service.h"
#include "net_io/codec/http/http_message.h"

namespace lt {
namespace net {

class H2CodecService : public CodecService {
public:
  // one http2 conneciton support multiple stream;
  class StreamCtx {
  public:
    StreamCtx(bool server, int32_t sid) {
      HttpMessage* message = nullptr;
      if (server) {
        req_ = std::make_shared<HttpRequest>();
        message = req_.get();
      } else {
        rsp_ = std::make_shared<HttpResponse>();
        message = rsp_.get();
      }
      message->SetStreamID(sid);
      message->SetKeepAlive(true);
    }
    ~StreamCtx() { };

    const RefHttpRequest& Request() { return req_; }
    const RefHttpResponse& Response() { return rsp_; }

    bool data_frame_send_ = false;
  private:
    RefHttpRequest req_;
    RefHttpResponse rsp_;
  };

  H2CodecService(base::MessageLoop*);

  ~H2CodecService();

  void StartProtocolService() override;

  void OnDataReceived(IOBuffer*) override;

  // last change for codec modify request
  void BeforeSendRequest(HttpRequest*);

  bool SendRequest(CodecMessage* message) override;

  // last change for codec modify response
  bool BeforeSendResponse(const HttpRequest*, HttpResponse*);

  bool SendResponse(const CodecMessage* req, CodecMessage* res) override;

  const RefCodecMessage NewResponse(const CodecMessage*) override;

  void HandleEvent(base::FdEvent* fdev, base::LtEv::Event ev) override;

  void DelStreamCtx(int32_t sid);
  StreamCtx* AddStreamCtx(int32_t sid);
  StreamCtx* FindStreamCtx(int32_t sid);

  void PushPromise(const std::string& method,
                   const std::string& req_path,
                   const HttpRequest* bind_req,
                   const RefHttpResponse& resp,
                   std::function<void(int status)> callback);
private:
  bool UseSSLChannel() const override;

  bool IsSessionClosed() const;

private:
  friend int on_begin_headers(nghttp2_session* session,
                              const nghttp2_frame* frame,
                              void* user_data);

  using HeaderKV = std::pair<std::string, std::string>;

  nghttp2_session* session_ = nullptr;

  std::map<int32_t, StreamCtx> stream_ctxs_;
};

}  // namespace net
}  // namespace lt

#endif
