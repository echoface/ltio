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

#ifndef _BASE_TIME_UTILS_H_
#define _BASE_TIME_UTILS_H_

#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

namespace base {

//millseconds ms  1000
//microseconds us 1000000
//nanoseconds  ns 1000000000
static const int64_t kNumMillisecsPerSec = INT64_C(1000);
static const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
static const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

static const int64_t kNumMicrosecsPerMillisec =
      kNumMicrosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMillisec =
      kNumNanosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMicrosec =
        kNumNanosecsPerSec / kNumMicrosecsPerSec;

int64_t SystemTimeNanos();
//ms
int64_t SystemTimeMillisecs();
//us
uint32_t time_us();
//ms
uint32_t time_ms();

uint32_t delta_ms(const uint32_t before_ms);

uint32_t delta_us(const uint32_t before_us);

struct timeval ms_to_timeval(uint32_t ms);

}
#endif
