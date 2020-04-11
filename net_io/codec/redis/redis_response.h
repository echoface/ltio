#ifndef _NET_PROTOCOL_REDIS_RESPONSE_H_
#define _NET_PROTOCOL_REDIS_RESPONSE_H_

#include <vector>
#include <memory>

#include <base/base_micro.h>
#include <thirdparty/resp/resp/all.hpp>
#include <net_io/codec/codec_message.h>

namespace lt {
namespace net {

class RespCodecService;
class RedisResponse;
typedef std::shared_ptr<RedisResponse> RefRedisResponse;

class RedisResponse : public CodecMessage {
  public:
    RedisResponse();
    ~RedisResponse();

    size_t Count() const {return results_.size();}
    const resp::unique_value& ResultAtIndex(size_t idx) const {return results_[idx];}

    bool IsHeartbeat() const override;
    std::string DebugDump() const;
    const std::string Dump() const override {return DebugDump();};
  private:
    friend class RespCodecService;
    void AddResult(resp::result& result);

    std::vector<resp::unique_value> results_;
};

}} //end namesapce
#endif
