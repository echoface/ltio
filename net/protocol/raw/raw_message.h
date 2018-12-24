#ifndef _NET_RAW_PROTOMESSAGE_H_H
#define _NET_RAW_PROTOMESSAGE_H_H

#include <cinttypes>
#include "net_callback.h"
#include "protocol/proto_message.h"

namespace net {

struct LtRawHeader {
public:
  static const uint64_t kHeaderSize;
  static const uint64_t kHeartBeatId;

  LtRawHeader* ToNetOrder();
  LtRawHeader* FromNetOrder();
  const std::string Dump() const;
  inline uint64_t identify_id() const {return sequence_id_;}

  /* use for status code*/
  uint8_t code = 0;
  /* Method of this message*/
  uint8_t method = 0;
private:
  friend class RawProtoService;
  /* Frame: frame_size = sizeof(LtRawHeader) + content_size*/
  uint64_t content_size_ = 0;
  /* 0: heart beat sequence id
   * other: application message*/
  uint64_t sequence_id_ = 0;
};

template <typename HeaderType>
class RawMessage : public ProtocolMessage {
public:
  typedef RawMessage<HeaderType> ResponseType;

  typedef RawMessage<HeaderType> ConcreteRawMessage;
  typedef std::shared_ptr<ConcreteRawMessage> RefRawMessage;

  static RefRawMessage Create(bool request) {
    auto t = request ? MessageType::kRequest : MessageType::kResponse;
    return std::make_shared<ConcreteRawMessage>(t);
  };

  RawMessage(MessageType t) :ProtocolMessage("raw", t) {};
  virtual ~RawMessage() {};

  const std::string& Content() const {return content_;}
  uint64_t ContentLength() const {return content_.size();}
  void SetContent(const std::string& c) {content_ = c;};
  void SetContent(const char* content) {content_ = content;}
  void AppendContent(const char* c, uint64_t len) {content_.append(c, len);}

  HeaderType* MutableHeader() {return &header_;}
  const HeaderType& Header() const {return header_;}

  const std::string Dump() const {
    std::ostringstream oss;

    oss << "{\"type\": \"" << TypeAsStr() << "\","
        << "\"header\": " << header_.Dump() << ","
        << "\"content\": \"" << content_ << "\""
        << "}";
    return std::move(oss.str());
  }
private:
  HeaderType header_;
  std::string content_;
};

typedef RawMessage<LtRawHeader> LtRawMessage;
}//end namespace net
#endif
