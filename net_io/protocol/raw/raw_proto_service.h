#ifndef NET_RAW_PROTO_SERVICE_H
#define NET_RAW_PROTO_SERVICE_H

#include "raw_message.h"

#include <net_io/protocol/proto_service.h>

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
          SendResponseMessage(message.get(), response.get());
        }
      } else if (delegate_) {
        delegate_->OnProtocolMessage(RefCast(ProtocolMessage, message));
      }
    } while (1);
  }

  const RefProtocolMessage NewResponse(const ProtocolMessage* req) override {
    CHECK(req->GetMessageType() == MessageType::kRequest);
    return RawMessageType::CreateResponse((RawMessageType*)req);
  }

  const RefProtocolMessage NewHeartbeat() {
    auto message = RawMessageType::Create(!IsServerSide());
    if (message->AsHeartbeat()) {
      return std::move(message);
    }
    return NULL;
  }

  //feature list
  bool KeepSequence() override { return RawMessageType::KeepQueue();};
  bool KeepHeartBeat() override {return RawMessageType::WithHeartbeat();}

  bool SendRequestMessage(ProtocolMessage* message) override {
    RawMessageType* request = static_cast<RawMessageType*>(message);
    CHECK(request->GetMessageType() == MessageType::kRequest);

    request->SetAsyncId(RawProtoService::sequence_id_++);
    return request->EncodeTo(channel_.get());
  };

  bool SendResponseMessage(const ProtocolMessage* req, ProtocolMessage* res) override {
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
std::atomic<uint64_t> RawProtoService<T>::sequence_id_ = {0};


}  // namespace net
}  // namespace lt
#endif
