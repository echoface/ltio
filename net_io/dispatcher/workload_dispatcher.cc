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

#include "workload_dispatcher.h"

#include <net_io/codec/codec_message.h>
#include "glog/logging.h"

namespace lt {
namespace net {

Dispatcher::Dispatcher(bool handle_in_io) : handle_in_io_(handle_in_io) {
  round_robin_counter_.store(0);
}

base::MessageLoop* Dispatcher::NextWorker() {
  if (workers_.empty()) {
    return NULL;
  }
  uint32_t index = ++round_robin_counter_ % workers_.size();
  return workers_[index];
}

bool Dispatcher::SetWorkContext(CodecMessage* message) {
  auto loop = base::MessageLoop::Current();
  message->SetWorkerCtx(loop);
  return loop != NULL;
}

bool Dispatcher::Dispatch(const base::LtClosure& clourse) {
  if (handle_in_io_) {
    clourse();
    return true;
  }

  base::MessageLoop* loop = NextWorker();
  if (nullptr == loop) {
    return false;
  }
  return loop->PostTask(FROM_HERE, clourse);
}

}  // namespace net
}  // namespace lt
