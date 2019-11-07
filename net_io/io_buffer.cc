#include "io_buffer.h"
#include <vector>
#include "glog/logging.h"

#include <unistd.h>
#include <termios.h>

#include <fcntl.h>
#include <cerrno>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <base/utils/sys_error.h>

static const char kCRLF[] = "\r\n";
static const int32_t kWarningBufferSize = 64 * 1024 * 1024;

namespace lt {
namespace net {

IOBuffer::IOBuffer(uint64_t init_size) :
  read_index_(0),
  write_index_(0),
  data_(init_size) {
}

IOBuffer::IOBuffer(const IOBuffer&& r) :
  read_index_(r.read_index_),
  write_index_(r.write_index_),
  data_(std::move(r.data_)) {
}

IOBuffer::~IOBuffer() {
  data_.clear();
  read_index_ = 0;
  write_index_ = 0;
}

bool IOBuffer::EnsureWritableSize(int64_t len) {

  if (int64_t(data_.size() - write_index_) >= len) {
    return true;
  }

  if (int64_t(CanWriteSize() + read_index_) >= len)  {

    uint64_t data_size = CanReadSize();
    std::copy(data_.begin() + read_index_,
              data_.begin() + write_index_,
              data_.begin());
    read_index_ = 0;
    write_index_ = data_size;
    LOG_IF(INFO, data_.size() > kWarningBufferSize) << "Buffer Size > 32M";
    CHECK(int64_t(CanWriteSize()) >= len);
  } else {
    //TODO:consider move data too in some case
    data_.resize(write_index_ + len);
  }
  return true;
}

inline char* IOBuffer::MutableRead() {
  return &data_[read_index_];
}
inline char* IOBuffer::MutableWrite() {
  return &data_[write_index_];
}

const char* IOBuffer::GetRead() {
  return &data_[read_index_];
}
char* IOBuffer::GetWrite() {
  return &data_[write_index_];
}

void IOBuffer::WriteString(const std::string& str) {
  EnsureWritableSize(str.size());
  memcpy(&data_[write_index_], str.data(), str.size());
  Produce(str.size());
}

void IOBuffer::WriteRawData(const void* data, size_t len) {
  EnsureWritableSize(len);
  memcpy(&data_[write_index_], data, len);
  Produce(len);
}

std::string IOBuffer::AsString() {
  std::string str(data_.begin() + read_index_, data_.begin() + write_index_);
  return std::move(str);
}

void IOBuffer::Consume(uint64_t len) {
  read_index_ += std::min(len, CanReadSize());
  if (read_index_ == write_index_) {
    read_index_ = write_index_ = 0;
  }
}


bool IOBuffer::HasALine() {
  return NULL !=  memchr(GetRead(), '\n', CanReadSize());
}

const char* IOBuffer::FindCRLF() {
  if (!CanReadSize()) {
    return NULL;
  }
  const char* res = std::search(MutableRead(), MutableWrite(), kCRLF, kCRLF+2);
  return res == GetWrite() ? NULL : res;
}

}}
