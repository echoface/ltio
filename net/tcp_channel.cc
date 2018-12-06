

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
    return OnConnectionReady();
  }

  auto task = std::bind(&TcpChannel::OnConnectionReady, shared_from_this());
  io_loop_->PostTask(NewClosure(std::move(task)));
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << channal_name_ << " Gone, Fd:" << fd_event_->fd();
  fd_event_.reset();
  CHECK(channel_status_ == DISCONNECTED);
}

void TcpChannel::OnConnectionReady() {
  CHECK(InIOLoop());

  if (channel_status_ != CONNECTING) {
    LOG(ERROR) << " Channel Status Changed After Enable this Channel";
    if (closed_callback_) {
      RefTcpChannel guard(shared_from_this());
      closed_callback_(guard);
    }
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

  int32_t bytes_read = in_buffer_.ReadFromSocketFd(fd_event_->fd(), &error);
  VLOG(GLOG_VTRACE) << " Read " << bytes_read << " bytes from:" << channal_name_;

  if (bytes_read > 0) {

    channel_consumer_->OnDataRecieved(shared_from_this(), &in_buffer_);

  } else if (0 == bytes_read) { //read eof, remote close
    VLOG(GLOG_VTRACE) << "peer closed:" << peer_addr_.IpPortAsString() << " local" << local_addr_.IpPortAsString() << " peer:" ;
    HandleClose();
  } else {
    LOG(ERROR) << "socket:" << fd_event_->fd() << " error:" << base::StrError(error);
    errno = error;
    HandleError();

    HandleClose();
  }
}

void TcpChannel::HandleWrite() {

  int socket_fd = fd_event_->fd();

  int32_t data_size = out_buffer_.CanReadSize();
  if (0 == data_size) {
    fd_event_->DisableWriting();

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(socket_fd);
    }
    return;
  }
  int writen_bytes = socketutils::Write(socket_fd, out_buffer_.GetRead(), data_size);

  if (writen_bytes > 0) {
    out_buffer_.Consume(writen_bytes);

    if (out_buffer_.CanReadSize() == 0) { //all data writen
      channel_consumer_->OnDataFinishSend(shared_from_this());
    }

  } else {
    LOG(ERROR) << "Call Socket Write Error";
  }

  if (out_buffer_.CanReadSize() == 0) { //all data writen
    fd_event_->DisableWriting();

    if (schedule_shutdown_) {
      socketutils::ShutdownWrite(fd_event_->fd());
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

  VLOG(GLOG_VTRACE) << "HandleClose, Channel:" << ChannelName() << " Status: DISCONNECTED";

  SetChannelStatus(DISCONNECTED);
  LOG(INFO) << "HandleClose, Channel:" << ChannelName() << " Status: DISCONNECTED";

  if (closed_callback_) {
    closed_callback_(guard);
  }
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

  int32_t n_write = 0;
  int32_t n_remain = len;

  //nothing write in out buffer
  if (0 == out_buffer_.CanReadSize()) {

    n_write = socketutils::Write(fd_event_->fd(), data, len);

    if (n_write >= 0) {

      n_remain = len - n_write;

      if (n_remain == 0) { //finish all

        RefTcpChannel guard(shared_from_this());
        channel_consumer_->OnDataFinishSend(guard);

      }
    } else { //n_write < 0

      n_write = 0;
      int32_t err = errno;
      LOG_IF(ERROR, EAGAIN != err) << "channel write data failed, fd:[" << fd_event_->fd() << "]" << "errno: [" << base::StrError(err) << "]";
    }
  }

  if (n_remain > 0) {
    if (!fd_event_->IsWriteEnable()) {
      fd_event_->EnableWriting();
    }
    out_buffer_.WriteRawData(data + n_write, n_remain);
  }
  return n_write;
}

void TcpChannel::SetChannelName(const std::string name) {
  channal_name_ = name;
}

void TcpChannel::SetCloseCallback(const ChannelClosedCallback& callback) {
  closed_callback_ = callback;
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
