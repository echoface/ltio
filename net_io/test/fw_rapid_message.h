#ifndef _LT_FW_RAPID_MESSAGE_H_
#define _LT_FW_RAPID_MESSAGE_H_

#include <protocol/proto_message.h>
#include <string>

using namespace lt;

typedef struct RapidHeader {
  uint32_t size = sizeof(RapidHeader); // total package length in bytes
  uint32_t seqid;
  uint8_t  type; //
  uint8_t  version;
  uint16_t cmdid;
  uint32_t extra;
}RapidHeader;

class FwRapidMessage : public lt::net::ProtocolMessage {
  public:
    typedef FwRapidMessage ResponseType;
    typedef std::shared_ptr<FwRapidMessage> RefFwRapidMessage;

    static RefFwRapidMessage Create(bool request) {
      auto t = request ? net::MessageType::kRequest : net::MessageType::kResponse;
      auto msg = std::make_shared<FwRapidMessage>(t);
      msg->header_.size = sizeof(RapidHeader);
      return msg;
    }

    static RefFwRapidMessage CreateResponse(FwRapidMessage* request) {
      auto res = Create(false);
      res->header_ = request->header_;
      res->header_.size = sizeof(RapidHeader);
      res->header_.extra = 0;
      return std::move(res);
    }

    static bool Encode(const FwRapidMessage* m, net::SocketChannel* ch) {
      if (ch->Send((const uint8_t*)(&m->header_), sizeof(RapidHeader)) < 0) {
        return false;
      }
      return ch->Send((const uint8_t*)m->content_.data(), m->content_.size()) >= 0;
    }

    static RefFwRapidMessage Decode(net::IOBuffer* buffer, bool server_side) {
      if (buffer->CanReadSize() < sizeof(RapidHeader)) {
        return NULL;
      }
      uint64_t frame_size = *(uint32_t*)(buffer->GetRead());
      if (buffer->CanReadSize() < frame_size) {
        return NULL;
      }
      auto message = Create(server_side);
      ::memcpy(&message->header_, buffer->GetRead(), sizeof(RapidHeader));
      const uint64_t body_size = message->header_.size - sizeof(RapidHeader);
      buffer->Consume(sizeof(RapidHeader));

      if (body_size > 0) {
        message->content_ = std::string((const char*)buffer->GetRead(), body_size);
        buffer->Consume(body_size);
      }
      return message;
    }

    FwRapidMessage(net::MessageType t) : ProtocolMessage(t) {}
    ~FwRapidMessage() {};

    RapidHeader* Header() {return &header_;}
    const std::string& Content() {return content_;}
    void SetContent(const std::string& c) {
      header_.size = sizeof(RapidHeader) + c.size();
      content_ = c;
    }

    //overide from protocolmessage
    void SetAsyncId(uint64_t id) override {header_.seqid = id;}
    const uint64_t AsyncId() const override {return header_.seqid;};
  private:
    RapidHeader header_;
    std::string content_;
};

#endif
