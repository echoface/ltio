#include "redis_response.h"

namespace net {

RedisResponse::RedisResponse() :
  ProtocolMessage(MessageType::kResponse) {
}

RedisResponse::~RedisResponse() {
}

void RedisResponse::AddResult(resp::result& result) {
  results_.push_back(result.value());
}

std::string RedisResponse::DebugDump() const {
  std::ostringstream oss;

  for (size_t i = 0; i < Count(); i++) {
    auto& value = ResultAtIndex(i);

    switch(value.type()) {
      case resp::ty_string: {
        oss << "index:" << i << " res:" << value.string().data() << "\n";
      } break;
      case resp::ty_error: {
        oss << "index:" << i << " res:" << value.error().data() << "\n";
      } break;
      case resp::ty_integer: {
        oss << "index:" << i << " res:" << value.integer() << "\n";
      } break;
      case resp::ty_null: {
        oss << "index:" << i << " res:" << "null" << "\n";
      } break;
      case resp::ty_array: {
        resp::unique_array<resp::unique_value> arr = value.array();

        std::ostringstream oss;
        for (size_t i = 0; i < arr.size(); i++) {
          oss << "'" << std::string(arr[i].bulkstr().data(), arr[i].bulkstr().size()) << "'";
          if (i < arr.size() - 1) {
            oss << ", ";
          }
        }
        oss << "index:" << i << " res:" << oss.str() << "\n";
      } break;
      case resp::ty_bulkstr: {
        oss << "index:" << i << " res:" << std::string(value.bulkstr().data(), value.bulkstr().size()) << "\n";
      } break;
      default: {
        oss << "index:" << i << " res:" << "#bad debug dump message#" << " type:" << value.type() << "\n";
      } break;
    }
  }
  return oss.str();
}

}//end namespace
