#ifndef NET_RAW_PROTO_SERVICE_H
#define NET_RAW_PROTO_SERVICE_H

#include "protocol/proto_service.h"
#include "raw_message.h"

namespace lt {
namespace net {

// just work on same endian machine; if not, consider add net-endian convert code
template <typename T>
class RawProtoService : public ProtoService {
 public:
  typedef T RawMessageType;
  typedef std::shared_ptr<T> RawMessageTypePtr;

  RawProtoService() : ProtoService() {}
  ~RawProtoService(){};

  void AfterChannelClosed() override { ; }
  // override from ProtoService
  void OnDataReceived(const SocketChannel*, IOBuffer* buffer) override {
    do {
      RawMessageTypePtr raw_message = RawMessageType::Decode(buffer, IsServerSide());
      if (!raw_message) {
        break;
      }
      if (delegate_) {
        raw_message->SetIOCtx(shared_from_this());
        delegate_->OnProtocolMessage(RefCast(ProtocolMessage, raw_message));
      }
    } while (1);
  }

  const RefProtocolMessage NewResponse(const ProtocolMessage* req) override {
    CHECK(req->GetMessageType() == MessageType::kRequest);
    return RawMessageType::CreateResponse((RawMessageType*)req);
  }

  bool KeepSequence() override { return false; };

  bool SendRequestMessage(const RefProtocolMessage& message) override {
    RawMessage* request = static_cast<RawMessage*>(message.get());
    CHECK(request->GetMessageType() == MessageType::kRequest);

    request->SetAsyncId(RawProtoService::sequence_id_++);
    return RawMessageType::Encode(request, channel_.get());
  };

  bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) override {
    auto raw_request = static_cast<RawMessage*>(req.get());
    auto raw_response = static_cast<RawMessage*>(res.get());

    CHECK(raw_request->AsyncId() == raw_response->AsyncId());
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " request:" << raw_request->Dump() << " response:" << raw_response->Dump();
    return RawMessageType::Encode(raw_response, channel_.get());
  };
 private:
  static std::atomic<uint64_t> sequence_id_;
};

template <typename T>
std::atomic<uint64_t> RawProtoService<T>::sequence_id_ = {0};


}  // namespace net
}  // namespace lt
#endif
