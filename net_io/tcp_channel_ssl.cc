#include <algorithm>

#include "base/utils/sys_error.h"
#include "net_io/socket_utils.h"
#include "tcp_channel_ssl.h"

namespace lt {
namespace net {

namespace {
const int32_t kBlockSize = 2 * 1024;

// handle openssl error afater ssl operation
// return value mean wheather should handle close event
void print_openssl_err() {
  int err;
  while ((err = ERR_get_error())) {
    const char* msg = (const char*)ERR_reason_error_string(err);
    const char* lib = (const char*)ERR_lib_error_string(err);
    const char* func = (const char*)ERR_func_error_string(err);
    LOG(ERROR) << "reason:" << msg << ", lib:" << lib << ", func:" << func;
  }
}

bool handle_openssl_err(base::FdEvent* ev, int err) {
  bool need_close = false;
  char err_str[256] = {0};
  switch (err) {
    case SSL_ERROR_WANT_READ: {
      ev->EnableReading();
    } break;
    case SSL_ERROR_WANT_WRITE: {
      ev->EnableWriting();
    } break;
    case SSL_ERROR_ZERO_RETURN:  // peer close write, can't read anything any
                                 // more
      need_close = true;
      break;
    case SSL_ERROR_SYSCALL:
      need_close = (errno != EAGAIN) && (errno != EINTR);
      break;
    default: {
      LOG(ERROR) << "openssl lib reason:" << ERR_lib_error_string(err);
      LOG(ERROR) << "openssl func reason:" << ERR_func_error_string(err);
      LOG(ERROR) << "openssl reason error:" << ERR_reason_error_string(err);
      need_close = true;
    } break;
  }
  return need_close;
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
  fd_event_->SetEdgeTrigger(true);
  socketutils::KeepAlive(socket_fd, true);
}

TCPSSLChannel::~TCPSSLChannel() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();
  SSL_free(ssl_);
}

void TCPSSLChannel::InitSSL(SSLImpl* ssl) {
  CHECK(ssl);
  ssl_ = ssl;
  int fd = SSL_get_fd(ssl);
  if (fd <= 0) {
    SSL_set_fd(ssl, fd_event_->GetFd());
  }
}

void TCPSSLChannel::StartChannel() {
  fd_event_->EnableReading();
  pump_->InstallFdEvent(fd_event_.get());

  SetChannelStatus(Status::CONNECTING);

  // NOTE: this will support auto negotiation when first read
  if (reciever_->IsServerSide()) {
    LOG(INFO) << "set as acceptor state";
    SSL_set_accept_state(ssl_);
  } else {
    LOG(INFO) << "set as connect state";
    SSL_set_connect_state(ssl_);
  }
  // if call SSL_set_accept_state(ssl_); SSL_set_connect_state(ssl_);
  // OnHandshake here are optional
  OnHandshake(fd_event_.get());
}

int32_t TCPSSLChannel::Send(const char* data, const int32_t len) {
  DCHECK(pump_->IsInLoop());

  if (out_buffer_.CanReadSize() > 0) {
    out_buffer_.WriteRawData(data, len);
    fd_event_->EnableWriting();
    return 0;  // all write to buffer, zero write to fd
  }

  size_t n_write = 0;
  size_t n_remain = len;

  bool need_close = false;
  do {
    int batch_size = std::min(size_t(kBlockSize), n_remain);

    ERR_clear_error();
    int ret = SSL_write(ssl_, data + n_write, batch_size);
    if (ret > 0) {
      n_write += ret;
      n_remain = n_remain - ret;

      DCHECK((n_write + n_remain) == size_t(len));
      continue;
    }
    if (errno == EINTR) {
      continue;
    }

    int err = SSL_get_error(ssl_, ret);
    need_close = handle_openssl_err(fd_event_.get(), err);
    LOG_IF(ERROR, need_close) << "ssl send fatal error, " << ChannelInfo();
    break;
  } while (1);

  if (need_close) {
    schedule_shutdown_ = true;
    SetChannelStatus(Status::CLOSING);
    fd_event_->EnableWriting();
  }

  return need_close ? -1 : n_write;
}

void TCPSSLChannel::ShutdownChannel(bool half_close) {
  DCHECK(pump_->IsInLoop());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

  schedule_shutdown_ = true;
  if (!half_close) {
    HandleClose(fd_event_.get());
    return;
  }

  if (!fd_event_->IsWriteEnable()) {
    fd_event_->EnableWriting();
  }
}

void TCPSSLChannel::ShutdownWithoutNotify() {
  DCHECK(pump_->IsInLoop());
  if (IsConnected()) {
    close_channel();
  }
}

bool TCPSSLChannel::OnHandshake(base::FdEvent* event) {
  LOG(INFO) << __FUNCTION__ << ", enter";
again:
  ERR_clear_error();
  int ret = SSL_do_handshake(ssl_);
  if (ret > 0) {  // handshake finish
    handshake_done_ = true;
    SetChannelStatus(Status::CONNECTED);
    reciever_->OnChannelReady(this);
    return true;
  }
  if (errno == EINTR) {
    goto again;
  }
  int err = SSL_get_error(ssl_, ret);
  LOG(INFO) << __FUNCTION__ << ", ret:" << ret << ", err:" << err;
  bool need_close = handle_openssl_err(event, err);
  if (need_close) {
    return HandleClose(event);
  }
  print_openssl_err();
  return true;
}

bool TCPSSLChannel::HandleWrite(base::FdEvent* event) {
  if (!handshake_done_) {
    return OnHandshake(event);
  }

  bool fatal_err = false;
  while (out_buffer_.CanReadSize()) {
    ERR_clear_error();
    int batch_size = std::min(uint64_t(kBlockSize), out_buffer_.CanReadSize());
    ssize_t retv = SSL_write(ssl_, out_buffer_.GetRead(), kBlockSize);
    if (retv > 0) {
      out_buffer_.Consume(retv);
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    int err = SSL_get_error(ssl_, retv);
    fatal_err = handle_openssl_err(event, err);
    break;
  };

  if (fatal_err) {
    LOG(ERROR) << "ssl write occurs fatal error, " << ChannelInfo();
    return HandleClose(event);
  }

  if (out_buffer_.CanReadSize() == 0) {
    fd_event_->DisableWriting();
    reciever_->OnDataFinishSend(this);

    if (schedule_shutdown_) {
      return HandleClose(event);
    }
  }
  return true;
}

bool TCPSSLChannel::HandleRead(base::FdEvent* event) {
  if (!handshake_done_) {
    return OnHandshake(event);
  }

  ssize_t bytes_read;

  bool need_close = false;
  do {
    in_buffer_.EnsureWritableSize(kBlockSize);

    ERR_clear_error();
    bytes_read =
        SSL_read(ssl_, in_buffer_.GetWrite(), in_buffer_.CanWriteSize());
    if (bytes_read > 0) {
      in_buffer_.Produce(bytes_read);
      continue;
    }
    if (errno == EINTR) {
      continue;
    }

    int err = SSL_get_error(ssl_, bytes_read);
    need_close = handle_openssl_err(event, err);
    LOG_IF(ERROR, need_close)
        << "ssl read occurs fatal error, " << ChannelInfo();
    break;
  } while (1);

  if (in_buffer_.CanReadSize()) {
    reciever_->OnDataReceived(this, &in_buffer_);
  }

  if (need_close) {
    SetChannelStatus(Status::CLOSING);
    return HandleClose(event);
  }
  return true;
}

}  // namespace net
}  // namespace lt
