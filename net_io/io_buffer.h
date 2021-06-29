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

#ifndef _NET_IO_BUFFER_H_H
#define _NET_IO_BUFFER_H_H

#include <string.h>
#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace lt {
namespace net {

#define kDefaultInitialSize 4096

class IOBuffer {
public:
  IOBuffer(uint64_t size = kDefaultInitialSize);
  IOBuffer(const IOBuffer&& r);
  ~IOBuffer();

  bool HasALine();
  const char* FindCRLF();
  bool EnsureWritableSize(int64_t len);

  char* GetWrite();
  const char* GetRead();
  inline uint64_t Empty() const { return CanReadSize() == 0; }
  inline uint64_t CanReadSize() const { return write_index_ - read_index_; }
  inline uint64_t CanWriteSize() const { return data_.size() - write_index_; }

  void WriteString(const std::string& str);
  void WriteRawData(const void* data, size_t len);

  template <typename T,
            typename = typename std::enable_if<std::is_pod<T>::value>::type>
  void Append(T data) {
    union __X {
      T _data;
      char _x;
    };
    uint32_t len = sizeof(data);
    EnsureWritableSize(len);
    ::memcpy(MutableWrite(), &data, len);
    Produce(len);
  }

  std::string AsString();

  void Consume(uint64_t len);
  inline void Produce(uint64_t len) { write_index_ += len; }

private:
  char* MutableRead();
  char* MutableWrite();

  uint64_t read_index_;
  uint64_t write_index_;
  std::vector<char> data_;
};

}  // namespace net
}  // namespace lt
#endif
