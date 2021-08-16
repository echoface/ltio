#include <algorithm>

#include "base/utils/sys_error.h"
#include "net_io/socket_utils.h"
#include "tcp_channel_ssl.h"

namespace lt {
namespace net {

namespace {
const int32_t kBlockSize = 8 * 1024;

enum SSLAction {
  Close,
  WaitIO,
  FastRetry,
};

std::string SSLActionStr(SSLAction action) {
  switch (action) {
    case Close:
      return "CLOSE";
    case WaitIO:
      return "WAITIO";
    case FastRetry:
      return "INTR";
  }
  return "BAD ACTION";
}

// handle openssl error afater ssl operation
// return value mean wheather should handle close event
void print_openssl_err() {
  int err;
  char err_str[128] = {0};
  while ((err = ERR_get_error())) {
    ERR_error_string_n(err, &err_str[0], arraysize(err_str));
    LOG(ERROR) << "ssl err:" << err_str;
  }
}

SSLAction handle_openssl_err(base::FdEvent* ev, int err) {
  bool dump_err = false;
  SSLAction action = SSLAction::Close;
  switch (err) {
    case SSL_ERROR_WANT_READ: {
      ev->EnableReading();
      VLOG(google::GLOG_INFO) << "ssl wait readable";
      action = SSLAction::WaitIO;
    } break;
    case SSL_ERROR_WANT_WRITE: {
      ev->EnableWriting();
      VLOG(google::GLOG_INFO) << "ssl wait writable";
      action = SSLAction::WaitIO;
    } break;
    case SSL_ERROR_ZERO_RETURN: {  // peer close write
      VLOG(google::GLOG_INFO) << "peer closed" << base::StrError();
      action = SSLAction::Close;
    } break;
    case SSL_ERROR_SYSCALL: {
      if (errno == EAGAIN) {
        action = SSLAction::WaitIO;
      } else if (errno == EINTR) {
        action = SSLAction::FastRetry;
      } else {
        LOG(ERROR) << "errno:" << errno << ", desc:" << base::StrError();
        dump_err = true;
        action = SSLAction::Close;
      }
    } break;
    default: {
      dump_err = true;
      action = SSLAction::Close;
    } break;
  }

  if (dump_err) {
    LOG(ERROR) << "ssl err:" << err << ", errno:" << base::StrError();
    print_openssl_err();
  }
  VLOG_IF(google::GLOG_INFO, action == SSLAction::Close) << "ssl ask for close";
  return action;
}

}  // namespace

// static
TCPSSLChannelPtr TCPSSLChannel::Create(int socket_fd,
                                       const IPEndPoint& local,
                                       const IPEndPoint& peer) {
  return TCPSSLChannelPtr(new TCPSSLChannel(socket_fd, local, peer));
}

TCPSSLChannel::TCPSSLChannel(int socket_fd,
                             const IPEndPoint& loc,
                             const IPEndPoint& peer)
  : SocketChannel(socket_fd, loc, peer) {
  socketutils::KeepAlive(socket_fd, true);
}

TCPSSLChannel::~TCPSSLChannel() {
  VLOG(VTRACE) << __FUNCTION__ << "@" << this << " Gone";
  SSL_free(ssl_);
}

void TCPSSLChannel::InitSSL(SSLImpl* ssl) {
  ssl_ = ssl;
}

bool TCPSSLChannel::StartChannel(bool server) {
  VLOG(VTRACE) << __FUNCTION__ << ", server:" << server;

  ignore_result(SocketChannel::StartChannel(server));

  if (server) {
    SSL_set_accept_state(ssl_);
  } else {
    SSL_set_connect_state(ssl_);
  }
  if (!DoHandshake(fdev_)) {
    return false;
  }
  return true;
}

int32_t TCPSSLChannel::Send(const char* data, const int32_t len) {

  if (out_.CanReadSize() > 0) {
    out_.WriteRawData(data, len);
    VLOG(VTRACE) << ChannelInfo() << ", append to outbuf, len:" << out_.CanReadSize();
    return HandleWrite();
  }

  ssize_t n_write = 0;
  ssize_t n_remain = len;
  SSLAction action = SSLAction::WaitIO;
  do {
    int batch_size = std::min(ssize_t(kBlockSize), n_remain);

    ERR_clear_error();
    int ret = SSL_write(ssl_, data + n_write, batch_size);
    if (ret > 0) {
      n_write += ret;
      n_remain = n_remain - ret;

      DCHECK((n_write + n_remain) == size_t(len));
      continue;
    }

    int err = SSL_get_error(ssl_, ret);
    action = handle_openssl_err(fdev_, err);

    if (action == SSLAction::WaitIO) {
      out_.WriteRawData(data + n_write, n_remain);
      break;
    }

    if (action == SSLAction::FastRetry) {
      VLOG(VTRACE) << "nintr, fast try again";
      continue;
    }

    // error handle
    LOG(ERROR) << ChannelInfo() << ", write err";
    return -1;

  } while (n_remain > 0);

  out_.CanReadSize() ? fdev_->EnableWriting() : fdev_->DisableWriting();

  VLOG(VTRACE) << ChannelInfo() << ", write:" << n_write;
  return n_write;
}

int TCPSSLChannel::HandleWrite() {
  // for ssl, do a write action without checking out buf size
  // bz it may in progress of handshake or re-negotionation
  SSLAction action = SSLAction::WaitIO;

  ssize_t total_write = 0;

  do {
    ERR_clear_error();
    int batch_size = std::min(uint64_t(kBlockSize), out_.CanReadSize());
    ssize_t retv = SSL_write(ssl_, out_.GetRead(), batch_size);
    if (retv > 0) {
      total_write += retv;
      out_.Consume(retv);
      VLOG(VTRACE) << ChannelInfo() << ", write:" << retv;
      continue;
    }

    int err = SSL_get_error(ssl_, retv);
    action = handle_openssl_err(fdev_, err);
    if (action ==  SSLAction::WaitIO) {
      VLOG(VTRACE) << ChannelInfo() << ", write info: want r/w";
      break;
    }

    if (action == SSLAction::FastRetry) {
      continue;
    }

    return -1;
  } while (out_.CanReadSize());

  if (!out_.CanReadSize()) {
    fdev_->DisableWriting();
  } else {
    fdev_->EnableWriting();
  }
  return total_write;
}

bool TCPSSLChannel::DoHandshake(base::FdEvent* event) {
  SSLAction action = SSLAction::WaitIO;
  do {
    ERR_clear_error();
    int ret = SSL_do_handshake(ssl_);
    if (ret > 0) {  // handshake finish
      VLOG(VTRACE) << ChannelInfo() << ", handshake finish";
      return true;
    }
    int err = SSL_get_error(ssl_, ret);
    action = handle_openssl_err(event, err);
    VLOG_IF(VERROR, action == SSLAction::Close) << ChannelInfo() << ", handshake fail";
    VLOG_IF(VTRACE, action == SSLAction::WaitIO) <<ChannelInfo() << ", handshake continue";
  } while (action == SSLAction::FastRetry);

  return action == SSLAction::Close ? false : true;
}

int TCPSSLChannel::HandleRead() {
  int bytes_read;

  SSLAction action = SSLAction::WaitIO;
  do {
    in_.EnsureWritableSize(2048);

    ERR_clear_error();
    bytes_read = SSL_read(ssl_, in_.GetWrite(), in_.CanWriteSize());
    if (bytes_read > 0) {
      in_.Produce(bytes_read);
      VLOG(VTRACE) << ChannelInfo() << ", read:" << bytes_read;
      continue;
    }

    int err = SSL_get_error(ssl_, bytes_read);
    action = handle_openssl_err(fdev_, err);
    if (action == SSLAction::WaitIO) {
      VLOG(VTRACE) << ChannelInfo() << ", read wait r/w";
      break;
    }
    if (action == SSLAction::FastRetry) {
      VLOG(VTRACE) << ChannelInfo() << ", read INTR";
      continue;
    }

    // SSLAction::CLOSE
    return -1;

  } while (1);

  return in_.CanReadSize();
}

}  // namespace net
}  // namespace lt
