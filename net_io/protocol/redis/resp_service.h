#ifndef _NET_PROTOCOL_RESP_SERVICE_H_H
#define _NET_PROTOCOL_RESP_SERVICE_H_H

#include "redis_response.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

class RedisResponse;
class RespService : public ProtoService {
public:
  typedef enum _ {
    kWaitAuth     = 0x01,
    kWaitSelectDB = 0x01 << 1,
  } InitWaitFlags;

  RespService();
  ~RespService();

  void OnStatusChanged(const SocketChannel*) override;
  void OnDataFinishSend(const SocketChannel*) override;

  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

	bool SendRequestMessage(const RefProtocolMessage &message) override;
	bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) override;
protected:
  void OnChannelReady(const SocketChannel*) override;

private:
  void HandleInitResponse(RedisResponse* response);
  uint8_t  init_wait_res_flags_ = 0;
  uint32_t next_incoming_count_ = 0;
  RefRedisResponse current_response;// = std::make_shared<RedisResponse>();
  resp::decoder decoder_;
};

}}// end namespace
#endif
