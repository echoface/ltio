/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "io_mux_kqueue.h"

#include <inttypes.h>

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
