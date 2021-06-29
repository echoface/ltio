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

#include "redis_response.h"

namespace lt {
namespace net {

RedisResponse::RedisResponse() : CodecMessage(MessageType::kResponse) {}

RedisResponse::~RedisResponse() {}

void RedisResponse::AddResult(resp::result& result) {
  results_.push_back(result.value());
}

bool RedisResponse::IsHeartbeat() const {
  if (Count() != 1) {
    return false;
  }
  auto& value = ResultAtIndex(0);
  return resp::ty_string == value.type() &&
         ::strncasecmp(value.string().data(), "PONG", 4) == 0;
}

std::string RedisResponse::DebugDump() const {
  std::ostringstream oss;
#define RESPBUF_TO_STRING(v) std::string(v.data(), v.size())

  for (size_t i = 0; i < Count(); i++) {
    auto& value = ResultAtIndex(i);

    switch (value.type()) {
      case resp::ty_error:
        oss << "index:" << i << " type:" << value.type()
            << " res:" << RESPBUF_TO_STRING(value.error()) << "\n";
        break;
      case resp::ty_string:
        oss << "index:" << i << " type:" << value.type()
            << " res:" << RESPBUF_TO_STRING(value.string()) << "\n";
        break;
      case resp::ty_bulkstr: {
        oss << "index:" << i << " type:" << value.type()
            << " res:" << RESPBUF_TO_STRING(value.bulkstr()) << "\n";
      } break;
      case resp::ty_integer: {
        oss << "index:" << i << " type:" << value.type()
            << " res:" << value.integer() << "\n";
      } break;
      case resp::ty_null: {
        oss << "index:" << i << " type:" << value.type() << " res:"
            << "null"
            << "\n";
      } break;
      case resp::ty_array: {
        resp::unique_array<resp::unique_value> arr = value.array();

        std::ostringstream os;
        for (size_t i = 0; i < arr.size(); i++) {
          switch (arr[i].type()) {
            case resp::ty_null:
              os << "null";
              break;
            case resp::ty_error:
              os << "'" << RESPBUF_TO_STRING(arr[i].error()) << "'";
              break;
            case resp::ty_string:
              os << "'" << RESPBUF_TO_STRING(arr[i].string()) << "'";
              break;
            case resp::ty_bulkstr:
              os << "'" << RESPBUF_TO_STRING(arr[i].bulkstr()) << "'";
              break;
            default:
              os << "'unknown type'";
              break;
          }
          if (i < arr.size() - 1) {
            os << ", ";
          }
        }
        oss << "index:" << i << " type:" << value.type() << " res:" << os.str()
            << "\n";
      } break;
      default: {
        oss << "index:" << i << " res:"
            << "#bad debug dump message#"
            << " type:" << value.type() << "\n";
      } break;
    }
  }
  return oss.str();
#undef RESPBUF_TO_STRING
}

}  // namespace net
}  // namespace lt
