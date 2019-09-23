//
// Created by gh on 18-12-5.
//
#include "channel.h"

// socket chennel interface and base class
namespace lt {
namespace net {

SocketChannel::SocketChannel(int socket_fd,
                             const SocketAddr& loc,
                             const SocketAddr& peer,
                             base::MessageLoop* loop)
  : io_loop_(loop),
    local_addr_(loc),
    peer_addr_(peer),
    name_(loc.IpPort()) {

  fd_event_ = base::FdEvent::Create(socket_fd,
                                    base::LtEv::LT_EVENT_NONE);
  socketutils::KeepAlive(fd_event_->fd(), true);
}

void SocketChannel::SetReciever(SocketChannel::Reciever* rec) {
  reciever_ = rec;
}

void SocketChannel::Start() {
  CHECK(fd_event_);
  CHECK(reciever_);

  status_ = Status::CONNECTING;

  auto t = std::bind(&SocketChannel::OnChannelReady, this);
  io_loop_->PostTask(NewClosure(std::move(t)));
}

void SocketChannel::OnChannelReady() {
  CHECK(InIOLoop());

  if (status_ != Status::CONNECTING) {
    LOG(ERROR) << __FUNCTION__ << ChannelInfo();
    fd_event_->ResetCallback();

    SetChannelStatus(Status::CLOSED);
    reciever_->OnChannelClosed(this);
    return;
  }

  fd_event_->EnableReading();
  base::EventPump* event_pump = io_loop_->Pump();
  event_pump->InstallFdEvent(fd_event_.get());
  SetChannelStatus(Status::CONNECTED);
  reciever_->OnChannelReady(this);
}

std::string SocketChannel::ChannelInfo() const {
  std::ostringstream oss;
  oss << " [name:" << name_
      << ", fd:" << fd_event_->fd()
      << ", loc:" << local_addr_.IpPort()
      << ", peer:" << peer_addr_.IpPort()
      << ", status:" << StatusAsString() << "]";
  return oss.str();
}

void SocketChannel::SetChannelStatus(Status st) {
  status_ = st;
  reciever_->OnStatusChanged(this);
}

const std::string SocketChannel::StatusAsString() const {
  switch(status_) {
    case Status::CONNECTING:
      return "CONNECTING";
    case Status::CONNECTED:
      return "ESTABLISHED";
    case Status::CLOSING:
      return "CLOSING";
    case Status::CLOSED:
      return "CLOSED";
    default:
    	break;
  }
  return "UNKNOWN";
}

}}

