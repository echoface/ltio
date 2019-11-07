#ifndef _NET_RAW_PROTOMESSAGE_H_H
#define _NET_RAW_PROTOMESSAGE_H_H

#include <cinttypes>
#include "protocol/proto_message.h"
#include "channel.h"

namespace lt {
namespace net {

typedef struct LtRawHeader {
  static const uint64_t kHeaderSize;
  LtRawHeader* ToNetOrder();
  LtRawHeader* FromNetOrder();
  const std::string Dump() const;
  /* 消息序列id,raw service支持异步消息*/
  inline uint64_t seq_id() const {return sequence_id_;}
  //payload size without header
  inline uint64_t payload_size() const {return content_size_;};
  //frame_size = header_size + frame_size
  inline uint64_t frame_size() const {return content_size_ + kHeaderSize;};

  /*status code*/
  uint8_t code = 0;
  /*Method of this message*/
  uint8_t method = 0;
  /* Frame: frame_size = sizeof(LtRawHeader) + content_size*/
  uint64_t content_size_ = 0;
  uint64_t sequence_id_ = 0;
}LtRawHeader;

/*
 * this is a raw message wrap class only support POD header data
 * POD header need provider a static self-decode method and a trait
 * of it's size
 *
 * for raw protoservice, any message need provide
 * Message::Create()
 * Message::CreateResponse()
 * Message::DecodeFrom()
 * Message::EncodeTo(channel)
 * Message::AsHeartbeat()
 * */
class RawMessage : public ProtocolMessage {
 public:
  typedef RawMessage ResponseType;
  typedef std::shared_ptr<RawMessage> RefRawMessage;

  static RefRawMessage Create(bool request);
  static RefRawMessage CreateResponse(const RawMessage* request);
  bool EncodeTo(SocketChannel* channel);
  static RefRawMessage Decode(IOBuffer* buffer, bool server_side);

  RawMessage(MessageType t);
  ~RawMessage(){};

  //override from ProtocalMessage
  void SetAsyncId(uint64_t id) override {header_.sequence_id_ = id;}
  const uint64_t AsyncId() const override {return header_.seq_id();};


  uint8_t Code() const {return header_.code;}
  void SetCode(const uint8_t code) {header_.code = code;}
  uint8_t Method() const {return header_.method;}
  void SetMethod(const uint8_t code) {header_.method = code;}

  void SetContent(const char* c);
  void SetContent(const std::string& c);
  void AppendContent(const char* c, uint64_t len);

  const std::string& Content() const { return content_;}
  uint64_t ContentLength() const { return content_.size(); }

  const std::string Dump() const;
private:
  LtRawHeader header_;
  std::string content_;
};
typedef RawMessage LtRawMessage;

}  // namespace net
}  // namespace lt
#endif
