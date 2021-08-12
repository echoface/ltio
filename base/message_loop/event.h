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

#ifndef BASE_LT_EVENT_H
#define BASE_LT_EVENT_H

#include <sys/epoll.h>
#include <sys/poll.h>

#include <functional>
#include <memory>

#include "base/lt_micro.h"

namespace base {

struct LtEv {
  using Event = uint8_t;

  enum __ev__ {
    NONE  = 0x00,
    READ  = 0x01,
    WRITE = 0x02,
    ERROR = 0x04,
  };

  static bool has_read(Event ev) {return ev | READ;};

  static bool has_write(Event ev) {return ev | WRITE;};

  static bool has_error(Event ev) {return ev | ERROR;};

  static std::string to_string(Event ev);
};

}  // namespace base
#endif
