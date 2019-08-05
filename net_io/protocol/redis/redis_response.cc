#include "redis_response.h"

namespace lt {
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
#define RESPBUF_TO_STRING(v) std::string(v.data(), v.size())

  for (size_t i = 0; i < Count(); i++) {
    auto& value = ResultAtIndex(i);

    switch(value.type()) {
      case resp::ty_error:
        oss << "index:" << i << " type:" << value.type() << " res:" << RESPBUF_TO_STRING(value.error()) << "\n";
        break;
      case resp::ty_string:
        oss << "index:" << i << " type:" << value.type() << " res:" << RESPBUF_TO_STRING(value.string()) << "\n";
        break;
      case resp::ty_bulkstr: {
        oss << "index:" << i << " type:" << value.type() << " res:" << RESPBUF_TO_STRING(value.bulkstr()) << "\n";
      } break;
      case resp::ty_integer: {
        oss << "index:" << i << " type:" << value.type() << " res:" << value.integer() << "\n";
      } break;
      case resp::ty_null: {
        oss << "index:" << i << " type:" << value.type() << " res:" << "null" << "\n";
      } break;
      case resp::ty_array: {
        LOG(INFO) << "got array result";
        resp::unique_array<resp::unique_value> arr = value.array();

        std::ostringstream os;
        for (size_t i = 0; i < arr.size(); i++) {
          switch(arr[i].type()) {
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
        oss << "index:" << i << " type:" << value.type() << " res:" << os.str() << "\n";
      } break;
      default: {
        oss << "index:" << i << " res:" << "#bad debug dump message#" << " type:" << value.type() << "\n";
      } break;
    }
  }
  return oss.str();
#undef RESPBUF_TO_STRING
}

}}//end namespace
