#ifndef _NET_PROTOCOL_RESP_SERVICE_H_H
#define _NET_PROTOCOL_RESP_SERVICE_H_H

#include "redis_response.h"
#include "protocol/proto_service.h"

namespace net {

class RespService : public ProtoService {
public:
  RespService();
  ~RespService();

  void OnStatusChanged(const RefTcpChannel&) override;
  void OnDataFinishSend(const RefTcpChannel&) override;

  void OnDataReceived(const RefTcpChannel &, IOBuffer *) override;

	bool SendRequestMessage(const RefProtocolMessage &message) override;
	bool ReplyRequest(const RefProtocolMessage& req, const RefProtocolMessage& res) override;
private:
  uint32_t next_incoming_count_ = 0;
  RefRedisResponse current_response;// = std::make_shared<RedisResponse>();
  resp::decoder decoder_;
};

}// end namespace
#endif
