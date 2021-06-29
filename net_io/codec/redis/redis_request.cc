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

#include "redis_request.h"

namespace lt {
namespace net {

RedisRequest::RedisRequest() : CodecMessage(MessageType::kRequest) {}

RedisRequest::~RedisRequest() {}

void RedisRequest::Get(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("GET", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::MGet(const std::vector<std::string>& keys) {
  std::vector<resp::buffer> buffers;
  encoder_.begin(buffers);
  auto cmder = encoder_.cmd("MGET");
  for (auto& key : keys) {
    cmder.arg(key);
  }
  cmder.end();
  encoder_.end();

  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Set(const std::string& key, const std::string& value) {
  std::vector<resp::buffer> buffers = encoder_.encode("SET", key, value);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::SetWithExpire(const std::string& key,
                                 const std::string& value,
                                 uint32_t second) {
  std::vector<resp::buffer> buffers =
      encoder_.encode("SET", key, value, "EX", std::to_string(second));

  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  // LOG(INFO) << __FUNCTION__ << " body:" << body_;
  cmd_counter_++;
}

void RedisRequest::MSet(
    const std::vector<std::pair<std::string, std::string>> kvs) {
  std::vector<resp::buffer> buffers;
  encoder_.begin(buffers);
  auto cmder = encoder_.cmd("MSET");
  for (auto& kv : kvs) {
    cmder.arg(kv.first).arg(kv.second);
  }
  cmder.end();
  encoder_.end();

  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Exists(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("EXISTS", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Delete(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("DEL", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Incr(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("INCR", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::IncrBy(const std::string& key, uint64_t by) {
  std::vector<resp::buffer> buffers =
      encoder_.encode("INCRBY", key, std::to_string(by));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::IncrByFloat(const std::string& key, float by) {
  std::vector<resp::buffer> buffers =
      encoder_.encode("INCRBYFLOAT", key, std::to_string(by));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Decr(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("DECR", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::DecrBy(const std::string& key, uint64_t by) {
  std::vector<resp::buffer> buffers =
      encoder_.encode("DECRBY", key, std::to_string(by));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Select(const std::string& db) {
  for (auto& buffer : encoder_.encode("SELECT", db)) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Auth(const std::string& password) {
  std::vector<resp::buffer> buffers = encoder_.encode("AUTH", password);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::TTL(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("TTL", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Persist(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("PERSIST", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Expire(const std::string& key, uint64_t second) {
  std::vector<resp::buffer> buffers =
      encoder_.encode("EXPIRE", key, std::to_string(second));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Ping() {
  for (auto& buffer : encoder_.encode("PING")) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

bool RedisRequest::AsHeartbeat() {
  body_.clear();
  cmd_counter_ = 0;
  Ping();
  return true;
}

bool RedisRequest::IsHeartbeat() const {
  ;
  return body_ == "PING";
}

}  // namespace net
}  // namespace lt
