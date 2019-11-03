

#include "tcp_channel.h"

#include "base/base_constants.h"
#include "glog/logging.h"
#include "base/closure/closure_task.h"
#include <base/utils/sys_error.h>

namespace lt {
namespace net {

//static
RefTcpChannel TcpChannel::Create(int socket_fd,
                                 const SocketAddr& local,
                                 const SocketAddr& peer,
                                 base::MessageLoop* loop) {

  //std::make_shared<TcpChannel>(socket_fd, local, peer, loop);
  return RefTcpChannel(new TcpChannel(socket_fd, local, peer, loop));
}

TcpChannel::TcpChannel(int socket_fd,
                       const SocketAddr& loc,
                       const SocketAddr& peer,
                       base::MessageLoop* loop)
  : SocketChannel(socket_fd, loc, peer, loop) {
  CHECK(io_loop_);

  fd_event_->SetReadCallback(std::bind(&TcpChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&TcpChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&TcpChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&TcpChannel::HandleError, this));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();
  CHECK(status_ == Status::CLOSED);
}

void TcpChannel::HandleRead() {
  static const int32_t block_size = 4 * 1024;
  do {
    in_buffer_.EnsureWritableSize(block_size);

    ssize_t bytes_read = ::read(fd_event_->fd(), in_buffer_.GetWrite(), in_buffer_.CanWriteSize());
    VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo() << " read [" << bytes_read << "] bytes";

    if (bytes_read > 0) {

      in_buffer_.Produce(bytes_read);
      reciever_->OnDataReceived(this, &in_buffer_);

    } else if (0 == bytes_read) {

      HandleClose();
      break;

    } else if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " read error:" << base::StrError();
      HandleClose();
      break;
    }
  } while(1);
}

void TcpChannel::HandleWrite() {
  int fatal_err = 0;
  int socket_fd = fd_event_->fd();

  while(out_buffer_.CanReadSize()) {

    ssize_t writen_bytes = socketutils::Write(socket_fd, out_buffer_.GetRead(), out_buffer_.CanReadSize());
    if (writen_bytes < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        fatal_err = errno;
      }
      break;
    }
    out_buffer_.Consume(writen_bytes);
  };

  if (fatal_err != 0) {
    LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " write err:" << base::StrError(fatal_err);
    HandleClose();
	  return;
  }

  if (out_buffer_.CanReadSize() == 0) {

    fd_event_->DisableWriting();
    reciever_->OnDataFinishSend(this);
    if (schedule_shutdown_) {
      HandleClose();
    }
  }
}

void TcpChannel::HandleError() {

  int err = socketutils::GetSocketError(fd_event_->fd());
  LOG(ERROR) << __FUNCTION__ << ChannelInfo() << " error: [" << base::StrError(err) << "]";

  HandleClose();
}

void TcpChannel::HandleClose() {
  DCHECK(io_loop_->IsInLoopThread());
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

  RefTcpChannel guard(shared_from_this());
  if (Status() == Status::CLOSED) {
    return;
  }
  close_channel();
}

void TcpChannel::ShutdownChannel(bool half_close) {
  CHECK(InIOLoop());
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();

  if (status_ == Status::CLOSED) {
    return;
  }

  schedule_shutdown_ = true;
  SetChannelStatus(Status::CLOSING);
  if (half_close && fd_event_->IsWriteEnable()) {
    schedule_shutdown_ = false;
  } else {
    HandleClose();
  }
}

int32_t TcpChannel::Send(const uint8_t* data, const int32_t len) {
  DCHECK(io_loop_->IsInLoopThread());

  if (!IsConnected()) {
    return -1;
  }

  if (out_buffer_.CanReadSize() > 0) {
    out_buffer_.WriteRawData(data, len);

    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
    return 0; //all write to buffer, zero write to fd
  }

  int32_t fatal_err = 0;
  size_t n_write = 0;
  size_t n_remain = len;

  do {
    ssize_t part_write = socketutils::Write(fd_event_->fd(), data + n_write, n_remain);

    if (part_write < 0) {

      if (errno == EAGAIN || errno == EWOULDBLOCK) {

        out_buffer_.WriteRawData(data + n_write, n_remain);

      } else {
        // to avoid A -> B -> callback  delete A problem, we use a writecallback handle this
      	fatal_err = errno;
        schedule_shutdown_ = true;
        SetChannelStatus(Status::CLOSING);
        if (!fd_event_->IsWriteEnable()) {
          fd_event_->EnableWriting();
        }
        LOG(ERROR) << ChannelInfo() << " send err:" << base::StrError() << " schedule close";
      }
      break;
    }
    n_write += part_write;
    n_remain = n_remain - part_write;

    DCHECK((n_write + n_remain) == size_t(len));
  } while(n_remain != 0);

  if (!fd_event_->IsWriteEnable() && out_buffer_.CanReadSize() > 0) {
    fd_event_->EnableWriting();
  }
  return fatal_err != 0 ? -1 : n_write;
}


}} //end lt::net
