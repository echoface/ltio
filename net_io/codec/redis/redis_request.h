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

#ifndef _NET_PROTOCOL_REDIS_MESSAGE_H_
#define _NET_PROTOCOL_REDIS_MESSAGE_H_

#include "redis_response.h"

#include <base/lt_macro.h>
#include <net_io/codec/codec_message.h>
#include <thirdparty/resp/resp/all.hpp>
#include <vector>

namespace lt {
namespace net {

class RespCodecService;
class RedisRequest;
typedef std::shared_ptr<RedisRequest> RefRedisRequest;

class RedisRequest : public CodecMessage {
public:
  typedef RedisResponse ResponseType;
  typedef resp::encoder<resp::buffer> RespEncoder;

  struct __detail__ {
    static void cmd_concat(RespEncoder::command&) { /* do nothing */
    }
    template <typename T, typename... Args>
    static void cmd_concat(RespEncoder::command& cmd,
                           const T& v,
                           Args&&... args) {
      cmd.arg(v);
      cmd_concat(cmd, std::forward<Args>(args)...);
    }
  };

  RedisRequest();
  ~RedisRequest();

  void Get(const std::string& key);
  void MGet(const std::vector<std::string>& keys);

  template <typename T>
  void MGet(std::initializer_list<T> list) {
    std::vector<resp::buffer> buffers;
    encoder_.begin(buffers);
    auto cmder = encoder_.cmd("MGET");
    for (auto& value : list) {
      cmder.arg(value);
    }
    cmder.end();
    encoder_.end();

    for (auto& buffer : buffers) {
      body_.append(buffer.data(), buffer.size());
    }
    cmd_counter_++;
  }

  template <typename... Args>
  void MGet(Args&&... args) {
    std::vector<resp::buffer> buffers;
    encoder_.begin(buffers);
    auto cmder = encoder_.cmd("MGET");

    __detail__::cmd_concat(cmder, std::forward<Args>(args)...);

    cmder.end();
    encoder_.end();
    for (auto& buffer : buffers) {
      body_.append(buffer.data(), buffer.size());
    }
    cmd_counter_++;
  }

  void Set(const std::string& key, const std::string& value);
  void MSet(const std::vector<std::pair<std::string, std::string>> kvs);

  void SetWithExpire(const std::string& key,
                     const std::string& value,
                     uint32_t second);

  void Exists(const std::string& key);
  void Delete(const std::string& key);

  void Incr(const std::string& key);
  void IncrBy(const std::string& key, uint64_t by);
  void IncrByFloat(const std::string& key, float by);
  void Decr(const std::string& key);
  void DecrBy(const std::string& key, uint64_t by);

  void Select(const std::string& db);
  void Auth(const std::string& password);

  void Ping();
  void TTL(const std::string& key);
  void Persist(const std::string& key);
  void Expire(const std::string& key, uint64_t second);
  /*
    void HDel(const std::string& hash_container, const std::string& field);
    void HSet(const std::string& hash_container, const std::string& field, const
    std::string& value); void HGet(const std::string& hash_container, const
    std::string& field); void HGetAll(const std::string& hash_container); void
    HExisit(const std::string& hash_container, const std::string& field); void
    HIncrBy(const std::string& hash_container, const std::string& field); void
    HIncrByFloat(const std::string& hash_container, const std::string& field);
    void HKeys(const std::string& hash_container);
    void HLen(const std::string& hash_container);
    void HMGet(const std::string& hash_container, std::vector<std::string>
    fields); void HMSet(const std::string& hash_container,
    std::vector<std::pair<std::string, std::string>> kvs); void
    HSetNotExist(const std::string& hash_container, const std::string& field,
    const std::string& value);
    //HScan
    //

    void SAdd(const std::string& set, const std::vector<std::string>& fields);
    void SMembers(const std::string& set, const std::string& field);
    void SDiff(const std::string& set1, const std::string& set2);
    void SIsMember(const std::string& set, const std::string& field);
  */

  inline uint32_t CmdCount() const { return cmd_counter_; }

  bool AsHeartbeat() override;
  bool IsHeartbeat() const override;

private:
  friend class RespCodecService;

  std::string body_;
  uint32_t cmd_counter_ = 0;
  resp::encoder<resp::buffer> encoder_;
};

}  // namespace net
}  // namespace lt
#endif
