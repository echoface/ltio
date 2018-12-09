#ifndef _NET_IO_BUFFER_H_H
#define _NET_IO_BUFFER_H_H

#include <vector>
#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <string>
#include <string.h>

namespace net {
#define kDefaultInitialSize 4096

class IOBuffer {
public:
  IOBuffer(uint64_t size = kDefaultInitialSize);
  IOBuffer(const IOBuffer&& r);
  ~IOBuffer();

  ssize_t ReadFromSocketFd(int socket, int *error);

  bool EnsureWritableSize(int64_t len);
  bool HasALine();

  uint8_t* GetWrite();
  const uint8_t* GetRead();

  inline uint64_t CanReadSize() {
    return write_index_ - read_index_;
  }
  inline uint64_t CanWriteSize() {
    return data_.size() - write_index_;
  }

  const uint8_t* FindCRLF();

  void WriteString(const std::string& str);
  void WriteRawData(const void* data, int32_t len);

  template <typename T> void Append(T data) {
    union _X {
      T _data;
      char _x;
    };
    uint32_t len = sizeof(data);
    EnsureWritableSize(len);
    ::memcpy(MutableWrite(), (uint8_t*) &data, len);
    Produce(len);
  }

  std::string AsString();

  void Consume(uint64_t len);

  inline void Produce(uint64_t len) {
    write_index_ += len;
  }
private:
  uint8_t* MutableRead();
  uint8_t* MutableWrite();

  uint64_t read_index_;
  uint64_t write_index_;
  std::vector<uint8_t> data_;
};

}
#endif
