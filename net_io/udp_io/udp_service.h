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

#ifndef _NET_IO_UDP_SERVICE_H_H_
#define _NET_IO_UDP_SERVICE_H_H_

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sys/socket.h>
#include <vector>
#include "udp_context.h"
#include <net_io/base/sockaddr_storage.h>
#include "base/message_loop/fd_event.h"

namespace lt {
namespace net {

class UDPService;
typedef std::shared_ptr<UDPService> RefUDPService;


#define LT_UDP_MTU_SIZE (2048-64*2)

typedef struct IOVecBuffer {
  IOVecBuffer() {
    LOG(INFO) << " construct buffer";
    iovec_.iov_base = buffer_;
    iovec_.iov_len  = LT_UDP_MTU_SIZE;
  }
  const char* Buffer() const {return buffer_;}

  struct iovec  iovec_;
  char  buffer_[LT_UDP_MTU_SIZE];
  SockaddrStorage src_addr_;
}IOVecBuffer;
//__attribute__ ((aligned (64)));

typedef struct UDPBufferDetail {
  UDPBufferDetail(struct mmsghdr* hdr, IOVecBuffer* buffer)
    : mmsg_hdr(hdr),
      buffer(buffer) {
  }
  nonstd::string_view data() const {
    if (mmsg_hdr == nullptr || buffer == nullptr) {
      return nonstd::string_view();
    }
    return nonstd::string_view(buffer->Buffer(), mmsg_hdr->msg_len);
  }

  struct mmsghdr* mmsg_hdr = nullptr;
  IOVecBuffer* buffer = nullptr;
} UDPBufferDetail;

class UDPPollBuffer final {
public:
  UDPPollBuffer(size_t count)
    : iovec_buf_(count),
      mmsghdr_(count) {
    for (int i = 0; i < mmsghdr_.size(); i++) {
      struct mmsghdr *msg = &mmsghdr_[i];
      IOVecBuffer& buffer = iovec_buf_[i];

      msg->msg_hdr.msg_control=0;
      msg->msg_hdr.msg_controllen=0;
      msg->msg_hdr.msg_iovlen = 1;
      msg->msg_hdr.msg_iov = &buffer.iovec_;
      msg->msg_hdr.msg_name = buffer.src_addr_.AsSockAddr();
      msg->msg_hdr.msg_namelen = buffer.src_addr_.Size();
    }
  }

  std::size_t Count() const {return mmsghdr_.size();};
  std::size_t BufferSize() const {return LT_UDP_MTU_SIZE;};

  UDPBufferDetail GetBufferDetail(size_t index) {
    return {&(mmsghdr_[index]), &iovec_buf_[index]};
  }

  struct mmsghdr* GetMmsghdr() {
    return &mmsghdr_[0];
  }
private:
  std::vector<struct mmsghdr> mmsghdr_;
  std::vector<IOVecBuffer> iovec_buf_;
};

class UDPService : public EnableShared(UDPService),
                   public base::FdEvent::Handler {
public:
  class Reciever {
    public:
      virtual void OnDataRecieve(UDPIOContextPtr context) = 0;
  };

public:
  ~UDPService() {};
  static RefUDPService Create(base::MessageLoop* io,
                              const IPEndPoint& ep);

  void StartService();
  void StopService();

private:
  UDPService(base::MessageLoop* io, const IPEndPoint& ep);

  //override from FdEvent::Handler
  bool HandleRead(base::FdEvent* fd_event) override;
  bool HandleWrite(base::FdEvent* fd_event) override;
  bool HandleError(base::FdEvent* fd_event) override;
  bool HandleClose(base::FdEvent* fd_event) override;

private:
  base::MessageLoop* io_;
  IPEndPoint endpoint_;
  base::RefFdEvent socket_event_;
  UDPPollBuffer buffers_;

  DISALLOW_COPY_AND_ASSIGN(UDPService);
};

}}
#endif
