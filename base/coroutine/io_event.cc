
#include "io_event.h"
#include "co_runner.h"
#include <glog/logging.h>

namespace co {

IOEvent::IOEvent(base::FdEvent* fdev)
  : pump_(CoroRunner::BindLoop()->Pump()),
    fd_event_(fdev) {
  initialize();
}

IOEvent::~IOEvent() {
  CHECK(pump_ == CoroRunner::BindLoop()->Pump());

  fd_event_->SetHandler(nullptr);
  pump_->RemoveFdEvent(fd_event_);
}

void IOEvent::initialize() {
  CHECK(__co_waitable__);
  CHECK(fd_event_->GetFd() > 0);

  fd_event_->SetHandler(this);
  if (fd_event_->MonitorEvents() == LtEv::NONE) {
    fd_event_->EnableReading();
    fd_event_->EnableWriting();
  }
  CHECK(pump_->InstallFdEvent(fd_event_));
}

std::string IOEvent::ResultStr() const {
  switch (result_) {
    case Result::Ok:
      return "ok";
    case Result::Error:
      return "error";
    case Result::Timeout:
      return "timeout";
    default:
      break;
  }
  return "none";
}

/*
* wait here till a fd event happened
* fd must be a non-blocking file-descript
* */
IOEvent::Result IOEvent::Wait(int ms) {
  CHECK(__co_waitable__);

  result_ = Result::None;

  resumer_ = std::move(co_new_resumer());

  if (ms > 0) {
    base::TimeoutEvent timer(ms, false);
    timer.SetAutoDelete(false);
    timer.InstallHandler(NewClosure([&](){
      set_result(Result::Timeout);
      return resumer_();
    }));
    pump_->AddTimeoutEvent(&timer);

    __co_wait_here__;

    pump_->RemoveTimeoutEvent(&timer);
  } else {

    __co_wait_here__;
  }

  resumer_ = nullptr;
  CHECK(result_ != Result::None) << ResultStr();

  return result_;
}

void IOEvent::HandleEvent(base::FdEvent* fdev, LtEv::Event ev) {
  if (fdev->MonitorEvents() & ev) {
    set_result(Result::Ok);
  } else {
    set_result(Result::Error);
  }
  return resumer_();
}

}
