

#include "tcp_channel.h"

#include "base/base_constants.h"
#include "glog/logging.h"
#include "base/closure/closure_task.h"
#include <base/utils/sys_error.h>

namespace net {


//static
RefTcpChannel TcpChannel::Create(int socket_fd,
                                 const InetAddress& local,
                                 const InetAddress& peer,
                                 base::MessageLoop* loop) {

  return std::make_shared<TcpChannel>(socket_fd, local, peer, loop);
}

TcpChannel::TcpChannel(int socket_fd,
                       const InetAddress& loc,
                       const InetAddress& peer,
                       base::MessageLoop* loop)
  : io_loop_(loop),
    local_addr_(loc),
    peer_addr_(peer) {

  CHECK(io_loop_);

  fd_event_ = base::FdEvent::Create(socket_fd, 0);
  fd_event_->SetReadCallback(std::bind(&TcpChannel::HandleRead, this));
  fd_event_->SetWriteCallback(std::bind(&TcpChannel::HandleWrite, this));
  fd_event_->SetCloseCallback(std::bind(&TcpChannel::HandleClose, this));
  fd_event_->SetErrorCallback(std::bind(&TcpChannel::HandleError, this));
  socketutils::KeepAlive(fd_event_->fd(), true);
}

void TcpChannel::Start() {
  CHECK(fd_event_);
  if (io_loop_->IsInLoopThread()) {
    return OnChannelReady();
  }
  auto task = std::bind(&TcpChannel::OnChannelReady, shared_from_this());
  io_loop_->PostTask(NewClosure(std::move(task)));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << channal_name_ << " Gone, Fd:" << fd_event_->fd();
  fd_event_.reset();
  CHECK(channel_status_ == DISCONNECTED);
}

void TcpChannel::OnChannelReady() {
  CHECK(InIOLoop());
  CHECK(channel_consumer_);

  if (channel_status_ != CONNECTING) {
    LOG(ERROR) << " Channel Status Changed After Enable this Channel";
    RefTcpChannel guard(shared_from_this());
    channel_consumer_->OnChannelClosed(guard);
    fd_event_->ResetCallback();
    return;
  }

  base::EventPump* event_pump = io_loop_->Pump();
  event_pump->InstallFdEvent(fd_event_.get());
  fd_event_->EnableReading();
  SetChannelStatus(CONNECTED);
}

void TcpChannel::SetChannelConsumer(ChannelConsumer *consumer) {
  channel_consumer_ = consumer;
}

void TcpChannel::HandleRead() {
  int error = 0;

  int64_t bytes_read = in_buffer_.ReadFromSocketFd(fd_event_->fd(), &error);
  if (bytes_read > 0) {
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " [" << ChannelName() << "] read ["
                      << fd_event_->fd() << "] got [" << bytes_read << "] bytes";

    channel_consumer_->OnDataRecieved(shared_from_this(), &in_buffer_);

  } else if (0 == bytes_read) { //read eof, remote close
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " peer [" << ChannelName() << "] closed, fd ["
                      << fd_event_->fd() << "]";
    HandleClose();

  } else {

    LOG(ERROR) << __FUNCTION__ << " [" << ChannelName() << "] socket ["
                      << fd_event_->fd() << "] error [" << base::StrError(error) << "]";
    errno = error;
    HandleError();

    HandleClose();
  }
}

void TcpChannel::HandleWrite() {

  int fatal_err = 0;
  int socket_fd = fd_event_->fd();

  while(out_buffer_.CanReadSize()) {

    ssize_t writen_bytes = socketutils::Write(socket_fd, out_buffer_.GetRead(), out_buffer_.CanReadSize());
    if (writen_bytes < 0) {
      int err = errno;
      if (err != EAGAIN && err != EWOULDBLOCK) {
        LOG(ERROR) << "channel [" << ChannelName() << "] handle write err:" << base::StrError(err);
        ForceShutdown();
        fatal_err = err;
      }
      break;
    }
    out_buffer_.Consume(writen_bytes);
  };

  if (fatal_err != 0) {
    return;
  }

  if (out_buffer_.CanReadSize()) {

  	if (!fd_event_->IsWriteEnable()) {
  	  fd_event_->EnableWriting();
  	}

  } else { //all data writen done

    channel_consumer_->OnDataFinishSend(shared_from_this());

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(socket_fd);
    } else {
      fd_event_->DisableWriting();
    }

  }
}

void TcpChannel::HandleError() {
  int socket_fd = fd_event_->fd();
  int err = socketutils::GetSocketError(socket_fd);
  LOG(ERROR) << "socket error, fd:[" << socket_fd << "], error: [" << base::StrError(err) << "]";
}

void TcpChannel::HandleClose() {
  CHECK(io_loop_->IsInLoopThread());

  RefTcpChannel guard(shared_from_this());

  fd_event_->DisableAll();
  fd_event_->ResetCallback();
  io_loop_->Pump()->RemoveFdEvent(fd_event_.get());


  SetChannelStatus(DISCONNECTED);

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " channel [" << ChannelName() << "] close";

  channel_consumer_->OnChannelClosed(guard);
}

void TcpChannel::SetChannelStatus(ChannelStatus st) {
  channel_status_ = st;
  RefTcpChannel guard(shared_from_this());
  channel_consumer_->OnStatusChanged(guard);
}

void TcpChannel::ShutdownChannel() {
  CHECK(InIOLoop());

  VLOG(GLOG_VTRACE) << "TcpChannel::ShutdownChannel " << channal_name_;
  if (channel_status_ == DISCONNECTED) {
    return;
  }

  if (!fd_event_->IsWriteEnable()) {
    SetChannelStatus(DISCONNECTING);
    socketutils::ShutdownWrite(fd_event_->fd());
  } else {
    schedule_shutdown_ = true;
  }
}

void TcpChannel::ForceShutdown() {
  CHECK(InIOLoop());
  HandleClose();
}

int32_t TcpChannel::Send(const uint8_t* data, const int32_t len) {
  CHECK(io_loop_->IsInLoopThread());
  if (channel_status_ != CONNECTED) {
    VLOG(GLOG_VERROR) << "Can't Write Data To a Closed[ing] socket; Channal:" << ChannelName();
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
      	fatal_err = errno;
        LOG(ERROR) << "channel [" << ChannelName() << "] write err:" << base::StrError(fatal_err);
        ForceShutdown();
      }
      break;
    }
    n_write += part_write;
    n_remain = n_remain - part_write;
    DCHECK((n_write + n_remain) == size_t(len));
  } while(n_remain != 0);

  if (out_buffer_.CanReadSize() > 0 && !fd_event_->IsWriteEnable()) {
    fd_event_->EnableWriting();
  }
  return fatal_err != 0 ? -1 : n_write;
}

void TcpChannel::SetChannelName(const std::string name) {
  channal_name_ = name;
}

const std::string TcpChannel::StatusAsString() const {
  switch(channel_status_) {
    case CONNECTING:
      return "CONNECTING";
    case CONNECTED:
      return "ESTABLISHED";
    case DISCONNECTING:
      return "DISCONNECTING";
    case DISCONNECTED:
      return "DISCONNECTED";
    default:
    	break;
  }
  return "UNKNOWN";
}

base::MessageLoop* TcpChannel::IOLoop() const {
  return io_loop_;
}
bool TcpChannel::InIOLoop() const {
  return io_loop_->IsInLoopThread();
}

bool TcpChannel::IsConnected() const {
  return channel_status_ == CONNECTED;
}

}
