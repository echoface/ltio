
#ifndef LIGHTINGIO_NET_SOCKET_RW_H
#define LIGHTINGIO_NET_SOCKET_RW_H

#include <cstdint>

#include "base/compiler_specific.h"
#include "base/lt_micro.h"
#include "net_io/io_buffer.h"

namespace co {
namespace so {

class SocketReader {
public:
  SocketReader(int socket);

  virtual ~SocketReader();

  // for initializing purpose, return false when error
  virtual bool Start() WARN_UNUSED_RESULT;

  int BindSocket() const { return socket; };

  // > 0 : bytes read from socket
  // = 0 : eof, peer socket closed
  // < 0 : error happened, log a detail error
  int Read(lt::net::IOBuffer* buf);

protected:
  int socket = -1;

  DISALLOW_COPY_AND_ASSIGN(SocketReader);
};

class SocketWriter {
public:
  SocketWriter(int socket);

  virtual ~SocketWriter();

  // for initializing purpose, return false when error
  virtual bool Start() WARN_UNUSED_RESULT;

  int BindSocket() const { return socket; };

  // write as much as data into socket
  // return true when, false when error
  // handle err is responsibility of caller
  // the caller should decide close channel or not
  virtual bool TryFlush(lt::net::IOBuffer* buf) WARN_UNUSED_RESULT = 0;

  // >= 0: bytes write to socket
  // < 0: error hanppened, log erro detail
  inline int32_t Send(const std::string& data) WARN_UNUSED_RESULT {
    return Send(data.data(), data.size());
  }

  // >= 0: bytes write to socket
  // < 0: error hanppened, log erro detail
  virtual int32_t Send(const char* data, int32_t len) WARN_UNUSED_RESULT = 0;

protected:
  int socket = -1;

  DISALLOW_COPY_AND_ASSIGN(SocketWriter);
};

}  // namespace so
}  // namespace co
#endif  // LIGHTINGIO_NET_CHANNEL_H
