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

#ifndef _BASE_IOMUX_KQUEUE_H_
#define _BASE_IOMUX_KQUEUE_H_

#include "event.h"
#include "io_multiplexer.h"

namespace base {

class IOMuxkQueue : public IOMux {
public:
  IOMuxkQueue();
  ~IOMuxkQueue();

  void AddFdEvent(FdEvent* fd_ev) override;
  void DelFdEvent(FdEvent* fd_ev) override;
  void UpdateFdEvent(FdEvent* fd_ev) override;
  int WaitingIO(FdEventList& active_list, int32_t timeout_ms) override;

private:
  int KQueueCtl(FdEvent* ev, int opt);
  LtEvent ToLtEvent(const uint32_t Kqueue_ev);
  uint32_t TokQueueEvent(const LtEvent& lt_ev, bool add_extr = true);
  std::string KQueueOptToString(int opt);

private:
  int Kqueue_fd_ = -1;
  std::vector<Kqueue_event> ep_events_;
};

}  // namespace base
#endif
