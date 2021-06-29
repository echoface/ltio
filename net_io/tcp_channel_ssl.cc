#include <algorithm>

#include "base/utils/sys_error.h"
#include "net_io/socket_utils.h"
#include "tcp_channel_ssl.h"

namespace lt {
namespace net {

namespace {
const int32_t kBlockSize = 4 * 1024;

static SSL_CTX* s_ssl_ctx = nullptr;

SSL_CTX* ssl_ctx_instance() {
  CHECK(s_ssl_ctx);
  return s_ssl_ctx;
}

void ssl_ctx_cleanup(SSL_CTX* ssl_ctx) {
  if (!ssl_ctx) {
    return;
  }
  if (ssl_ctx == s_ssl_ctx) {
    s_ssl_ctx = NULL;
  }
  SSL_CTX_free((SSL_CTX*)ssl_ctx);
  ssl_ctx = NULL;
}

SSL* ssl_new(SSL_CTX* ssl_ctx, int fd) {
  SSL* ssl = SSL_new(ssl_ctx);
  if (ssl == NULL)
    return NULL;
  SSL_set_fd(ssl, fd);
  return ssl;
}

void ssl_free(SSL* ssl) {
  if (ssl) {
    SSL_free((SSL*)ssl);
    ssl = NULL;
  }
}
}  // end namespace

#ifndef SSL_SUCCESS
#define SSL_SUCCESS 1
#endif

SSL_CTX* ssl_ctx_init(const char* cert, const char* key) {
  if (s_ssl_ctx != nullptr)
    return s_ssl_ctx;

  SSL_library_init();
  SSL_load_error_strings();

  s_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
  if (!s_ssl_ctx) {
    LOG(FATAL) << "Failed to create SSL context";
    goto err;
  }

  /* loads the first certificate stored in file into ctx */
  if (SSL_CTX_use_certificate_file(s_ssl_ctx, cert, SSL_FILETYPE_PEM) !=
      SSL_SUCCESS) {
    LOG(FATAL) << "OpenSSL Error: loading certificate file failed";
    goto err;
  }

  /*
   * adds the first private RSA key found in file to ctx.
   *
   * checks the consistency of a private key with the corresponding
   * certificate loaded into ctx. If more than one key/certificate
   * pair (RSA/DSA) is installed, the last item installed will be checked.
   */
  if (SSL_CTX_use_RSAPrivateKey_file(s_ssl_ctx, key, SSL_FILETYPE_PEM) !=
      SSL_SUCCESS) {
    LOG(FATAL) << "OpenSSL Error: loading key failed";
    goto err;
  }
  return s_ssl_ctx;
err:
  ssl_ctx_cleanup(s_ssl_ctx);
  return NULL;
}

// static
TCPSSLChannelPtr TCPSSLChannel::Create(int socket_fd,
                                       const IPEndPoint& local,
                                       const IPEndPoint& peer,
                                       base::EventPump* pump) {
  return TCPSSLChannelPtr(new TCPSSLChannel(socket_fd, local, peer, pump));
}

TCPSSLChannel::TCPSSLChannel(int socket_fd,
                             const IPEndPoint& loc,
                             const IPEndPoint& peer,
                             base::EventPump* pump)
  : SocketChannel(socket_fd, loc, peer, pump) {
  fd_event_->SetEdgeTrigger(true);
  socketutils::KeepAlive(socket_fd, true);
}

TCPSSLChannel::~TCPSSLChannel() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

  ssl_free(ssl_);
}

void TCPSSLChannel::StartChannel() {
  ssl_ctx_ = ssl_ctx_instance();
  CHECK(ssl_ctx_);

  ssl_ = ssl_new(ssl_ctx_, fd_event_->fd());

  fd_event_->EnableReading();
  pump_->InstallFdEvent(fd_event_.get());
  SetChannelStatus(Status::CONNECTING);

  // NOTE: this will support auto negotiation when first read
  if (reciever_->IsServerSide()) {
    SSL_set_accept_state(ssl_);
  } else {
    SSL_set_connect_state(ssl_);
  }
  // if call SSL_set_accept_state(ssl_); SSL_set_connect_state(ssl_);
  // OnHandshake here are optional
  OnHandshake(fd_event_.get());
}

int32_t TCPSSLChannel::Send(const char* data, const int32_t len) {
  DCHECK(pump_->IsInLoopThread());

  if (!IsConnected()) {
    return -1;
  }

  if (out_buffer_.CanReadSize() > 0) {
    out_buffer_.WriteRawData(data, len);

    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
    return 0;  // all write to buffer, zero write to fd
  }

  int32_t fatal_err = 0;
  size_t n_write = 0;
  size_t n_remain = len;

  do {
    int batch_size = std::min(size_t(kBlockSize), n_remain);
    int part_write = SSL_write(ssl_, data + n_write, batch_size);
    if (part_write > 0) {
      n_write += part_write;
      n_remain = n_remain - part_write;

      DCHECK((n_write + n_remain) == size_t(len));
      continue;
    }

    // part_write <= 0
    ERR_clear_error();
    int err = SSL_get_error(ssl_, part_write);
    if (err == SSL_ERROR_WANT_WRITE) {
      if (!fd_event_->IsWriteEnable()) {
        fd_event_->EnableWriting();
      }
      out_buffer_.WriteRawData(data + n_write, n_remain);
    } else if (err == SSL_ERROR_WANT_READ) {
      if (!fd_event_->IsReadEnable()) {
        fd_event_->EnableReading();
      }
      out_buffer_.WriteRawData(data + n_write, n_remain);
    } else {
      // to avoid A -> B -> callback delete A problem,
      // use a write event to triggle handle close action
      fatal_err = err;
      schedule_shutdown_ = true;
      SetChannelStatus(Status::CLOSING);
      if (!fd_event_->IsWriteEnable()) {
        fd_event_->EnableWriting();
      }
      LOG(ERROR) << ChannelInfo()
                 << " ssl err:" << ERR_reason_error_string(err);
    }
    break;
  } while (n_remain != 0);

  if (out_buffer_.CanReadSize() && (!fd_event_->IsWriteEnable())) {
    fd_event_->EnableWriting();
  }
  return fatal_err != 0 ? -1 : n_write;
}

void TCPSSLChannel::ShutdownChannel(bool half_close) {
  DCHECK(pump_->IsInLoopThread());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

  schedule_shutdown_ = true;
  if (half_close) {
    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
  } else {
    HandleClose(fd_event_.get());
  }
}

void TCPSSLChannel::ShutdownWithoutNotify() {
  DCHECK(pump_->IsInLoopThread());
  if (IsConnected()) {
    close_channel();
  }
}

bool TCPSSLChannel::OnHandshake(base::FdEvent* event) {
  int ret = reciever_->IsServerSide() ? SSL_accept(ssl_) : SSL_connect(ssl_);
  if (ret == SSL_SUCCESS) {  // handshake finish
    handshake_done_ = true;
    reciever_->OnChannelReady(this);
    return true;
  }

  ERR_clear_error();
  int err = SSL_get_error(ssl_, ret);
  if (err == SSL_ERROR_WANT_READ) {
    if (!event->IsReadEnable()) {
      event->EnableReading();
    }

  } else if (err == SSL_ERROR_WANT_WRITE) {
    if (!event->IsWriteEnable()) {
      event->EnableWriting();
    }

  } else {
    LOG(ERROR) << "SSLHandshake fail, error:" << ERR_reason_error_string(err);
    return HandleClose(event);
  }
  return true;
}

bool TCPSSLChannel::HandleWrite(base::FdEvent* event) {
  if (!handshake_done_) {
    return OnHandshake(event);
  }

  int fatal_err = SSL_ERROR_NONE;

  while (out_buffer_.CanReadSize()) {
    ERR_clear_error();

    int batch_size = std::min(uint64_t(kBlockSize), out_buffer_.CanReadSize());
    ssize_t retv = SSL_write(ssl_, out_buffer_.GetRead(), kBlockSize);
    if (retv > 0) {
      out_buffer_.Consume(retv);
      continue;
    }

    // if (retv <= 0) {
    int err = SSL_get_error(ssl_, retv);

    if (err == SSL_ERROR_WANT_WRITE) {
      if (!fd_event_->IsWriteEnable()) {
        fd_event_->EnableWriting();
      }
    } else if (err == SSL_ERROR_WANT_READ) {
      if (!fd_event_->IsReadEnable()) {
        fd_event_->EnableReading();
      }
    } else {
      // to avoid A -> B -> callback delete A problem,
      // use a write event to triggle handle close action
      fatal_err = err;
    }
    break;
  };

  if (fatal_err != SSL_ERROR_NONE) {
    LOG(ERROR) << ChannelInfo()
               << " write err:" << ERR_reason_error_string(fatal_err);
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

  int err = 0;
  ssize_t bytes_read;
  bool need_close = false;
  do {
    in_buffer_.EnsureWritableSize(kBlockSize);

    bytes_read =
        SSL_read(ssl_, in_buffer_.GetWrite(), in_buffer_.CanWriteSize());
    if (bytes_read > 0) {
      in_buffer_.Produce(bytes_read);
      continue;
    }

    ERR_clear_error();
    int err = SSL_get_error(ssl_, bytes_read);
    if (err == SSL_ERROR_WANT_WRITE) {
      if (!fd_event_->IsWriteEnable()) {
        fd_event_->EnableWriting();
      }
    } else if (err == SSL_ERROR_WANT_READ) {
      if (!fd_event_->IsReadEnable()) {
        fd_event_->EnableReading();
      }
    } else {
      LOG(ERROR) << ChannelInfo()
                 << " ssl err:" << ERR_reason_error_string(err);
      need_close = true;
      SetChannelStatus(Status::CLOSING);
    }
    break;
  } while (1);

  if (in_buffer_.CanReadSize()) {
    reciever_->OnDataReceived(this, &in_buffer_);
  }

  bool ev_continue = true;
  if (need_close) {
    ev_continue = HandleClose(event);
  }
  return ev_continue;
}

}  // namespace net
}  // namespace lt
