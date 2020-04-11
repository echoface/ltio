#ifndef NET_RAW_PROTO_SERVICE_H
#define NET_RAW_PROTO_SERVICE_H

#include "base/message_loop/message_loop.h"
#include "raw_message.h"

#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

// just work on same endian machine; if not, consider add net-endian convert code
template <typename T>
class RawCodecService : public CodecService {
 public:
  typedef T RawMessageType;
  typedef std::shared_ptr<T> RawMessageTypePtr;

  RawCodecService(base::MessageLoop* loop)
    : CodecService(loop) {
  }
  ~RawCodecService(){};

  void AfterChannelClosed() override { ; }
  // override from CodecService
  void OnDataReceived(const SocketChannel*, IOBuffer* buffer) override {
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";
    do {
      RawMessageTypePtr message = RawMessageType::Decode(buffer, IsServerSide());
      if (!message) {
        break;
      }
      message->SetIOCtx(shared_from_this());
      VLOG(GLOG_VTRACE) << __FUNCTION__ << "  decode a message success";

      if (IsServerSide() && message->IsHeartbeat()) {
        auto response = NewHeartbeat();
        response->SetAsyncId(message->AsyncId());
        if (response) {
          EncodeResponseToChannel(message.get(), response.get());
        }
      } else if (delegate_) {
        delegate_->OnCodecMessage(RefCast(CodecMessage, message));
      }
    } while (1);
  }

  const RefCodecMessage NewResponse(const CodecMessage* req) override {
    CHECK(req->GetMessageType() == MessageType::kRequest);
    return RawMessageType::CreateResponse((RawMessageType*)req);
  }

  const RefCodecMessage NewHeartbeat() {
    auto message = RawMessageType::Create(!IsServerSide());
    if (message->AsHeartbeat()) {
      return std::move(message);
    }
    return NULL;
  }

  //feature list
  bool KeepSequence() override { return RawMessageType::KeepQueue();};
  bool KeepHeartBeat() override {return RawMessageType::WithHeartbeat();}

  bool EncodeToChannel(CodecMessage* message) override {
    RawMessageType* request = static_cast<RawMessageType*>(message);
    CHECK(request->GetMessageType() == MessageType::kRequest);

    request->SetAsyncId(RawCodecService::sequence_id_++);
    return request->EncodeTo(channel_.get());
  };

  bool EncodeResponseToChannel(const CodecMessage* req, CodecMessage* res) override {
    auto raw_response = static_cast<RawMessageType*>(res);
    auto raw_request = static_cast<const RawMessageType*>(req);
    CHECK(raw_request->AsyncId() == raw_response->AsyncId());
    VLOG(GLOG_VTRACE) << __FUNCTION__
      << " request:" << raw_request->Dump()
      << " response:" << raw_response->Dump();
    return raw_response->EncodeTo(channel_.get());
  };
 private:
  static std::atomic<uint64_t> sequence_id_;
};

template <typename T>
std::atomic<uint64_t> RawCodecService<T>::sequence_id_ = {0};


}  // namespace net
}  // namespace lt
#endif
