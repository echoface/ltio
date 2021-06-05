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

#include "time_utils.h"

namespace base {

namespace {
  int64_t monotonic_nano_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return kNumNanosecsPerSec * static_cast<int64_t>(ts.tv_sec) + static_cast<int64_t>(ts.tv_nsec);
  }
}

//us
int64_t time_us() {
  return monotonic_nano_sec() / kNumNanosecsPerMicrosec;
}

//ms
int64_t time_ms() {
  return monotonic_nano_sec() / kNumNanosecsPerMillisec;
}

int64_t delta_ms(const int64_t before_ms) {
  return time_ms() - before_ms;
}

int64_t delta_us(const int64_t before_us) {
  return time_us() - before_us;
}

struct timeval ms_to_timeval(uint32_t ms) {
  timeval tv = {
    .tv_sec = ms / kNumMillisecsPerSec,
    .tv_usec = (ms % kNumMicrosecsPerMillisec) * kNumMicrosecsPerMillisec
  };
  //tv.tv_sec = ms / kNumMillisecsPerSec;
  //tv.tv_usec = (ms % kNumMicrosecsPerMillisec) * kNumMicrosecsPerMillisec;
  return tv;
}

}//end namespace
