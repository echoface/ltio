#ifndef _NET_PROTOCOL_RESP_SERVICE_H_H
#define _NET_PROTOCOL_RESP_SERVICE_H_H

#include "redis_response.h"
#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

class RedisResponse;
/*
 * A RESP protocol client side codec service
 * Note: not support as service side service
 *
 * - 支持客户端对db/auth等初始化操作
 * - 支持以Redis Ping命令作为心跳请求来与服务端保持活跃确认
 * */
class RespCodecService : public CodecService {
public:
  typedef enum _ {
    kWaitNone     = 0x0,
    kWaitAuth     = 0x01,
    kWaitSelectDB = 0x01 << 1,
  } InitWaitFlags;

  RespCodecService(base::MessageLoop* loop);
  ~RespCodecService();

  void OnDataFinishSend(const SocketChannel*) override;

  void OnDataReceived(const SocketChannel*, IOBuffer *) override;

	bool EncodeToChannel(CodecMessage* message) override;
	bool EncodeResponseToChannel(const CodecMessage* req, CodecMessage* res) override;

  bool KeepHeartBeat() override {return true;}
  const RefCodecMessage NewHeartbeat() override;
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
