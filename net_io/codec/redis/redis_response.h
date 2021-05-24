/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
