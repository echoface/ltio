#include "net_endian.h"
#include "raw_message.h"

namespace net {

const uint64_t LtRawHeader::kHeartBeatId = 0;
const uint64_t LtRawHeader::kHeaderSize = sizeof(LtRawHeader);

LtRawHeader* LtRawHeader::ToNetOrder() {
  sequence_id_ = endian::HostToNetwork64(sequence_id_);
  content_size_ = endian::HostToNetwork64(content_size_);
  return this;
}
LtRawHeader* LtRawHeader::FromNetOrder() {
  sequence_id_ = endian::NetworkToHost64(sequence_id_);
  content_size_ = endian::NetworkToHost64(content_size_);
  return this;
}

const std::string LtRawHeader::Dump() const {
  std::ostringstream oss;
  oss << "{\"code\": \"" << int(code) << "\","
      << "\"method\": " << int(method) << ","
      << "\"content_sz\": " << content_size_ << ","
      << "\"sequence_id\": \"" << sequence_id_ << "\""
      << "}";
  return std::move(oss.str());
}

}//end net
