#include "io_mux_kqueue.h"

namespace base {

IOMuxkQueue::IOMuxkQueue() {

}
IOMuxkQueue::~IOMuxkQueue() {

}

void IOMuxkQueue::AddFdEvent(FdEvent* fd_ev) {

}
void IOMuxkQueue::DelFdEvent(FdEvent* fd_ev) {

}

void IOMuxkQueue::UpdateFdEvent(FdEvent* fd_ev) {

}

int IOMuxkQueue::WaitingIO(FdEventList& active_list, int32_t ms) {
  return 0;
}

int IOMuxkQueue::KQueueCtl(FdEvent* ev, int opt) {
  return 0;
}

LtEvent IOMuxkQueue::ToLtEvent(const uint32_t Kqueue_ev) {

}

uint32_t IOMuxkQueue::TokQueueEvent(const LtEvent& lt_ev, bool add_extr) {
  return 0;
}

std::string IOMuxkQueue::KQueueOptToString(int opt) {

}

}//end base::
