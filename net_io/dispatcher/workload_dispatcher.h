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

#ifndef NET_WORKLOAD_DISPATCHER_H
#define NET_WORKLOAD_DISPATCHER_H

#include <base/message_loop/message_loop.h>
#include <net_io/codec/codec_message.h>
#include <net_io/net_callback.h>
#include <atomic>
#include <cinttypes>

namespace lt {
namespace net {

typedef std::vector<base::MessageLoop*> LoopList;

class Dispatcher {
public:
  Dispatcher(bool handle_in_io);
  virtual ~Dispatcher(){};

  void SetWorkerLoops(LoopList& loops) { workers_ = loops; };

  // must call at Worker Loop, may ioworker
  // or woker according to handle_in_io_
  virtual bool SetWorkContext(CodecMessage* message);
  // transmit task from IO TO worker loop
  virtual bool Dispatch(const base::LtClosure& closuse);

protected:
  const bool handle_in_io_;
  base::MessageLoop* NextWorker();

private:
  std::vector<base::MessageLoop*> workers_;
  std::atomic<uint64_t> round_robin_counter_;
};

}  // namespace net
}  // namespace lt
#endif
