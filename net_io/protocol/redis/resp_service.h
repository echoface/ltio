#ifndef _NET_PROTOCOL_RESP_SERVICE_H_H
#define _NET_PROTOCOL_RESP_SERVICE_H_H

#include "redis_response.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

class RespService : public ProtoService {
public:
  RespService();
  ~RespService();

  void OnStatusChanged(const SocketChannel*) override;
  void OnDataFinishSend(const SocketChannel*) override;

  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

	bool SendRequestMessage(const RefProtocolMessage &message) override;
	bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) override;
private:
  uint32_t next_incoming_count_ = 0;
  RefRedisResponse current_response;// = std::make_shared<RedisResponse>();
  resp::decoder decoder_;
};

}}// end namespace
#endif
