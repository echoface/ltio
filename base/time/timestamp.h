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

#ifndef BASE_TIMESTAMP_H_H
#define BASE_TIMESTAMP_H_H

#include <string>
#include "time_utils.h"

namespace base {
// time ensure the us always less than 1000000
class Timestamp {
public:
  Timestamp(const timeval tv);
  Timestamp(time_t sec, time_t us);
  Timestamp(const Timestamp& other);

  static Timestamp Now();
  static Timestamp NSecondLater(time_t sec);
  static Timestamp NMillisecondLater(time_t ms);
  static Timestamp NMicrosecondLater(time_t us);

  int64_t AsMillsecond() const;
  int64_t AsMicroSecond() const;
  std::string ToString();

  bool operator!=(const Timestamp& other) const;
  bool operator==(const Timestamp& other) const;
  bool operator>(const Timestamp& other) const;
  bool operator<(const Timestamp& other) const;
  bool operator<=(const Timestamp& other) const;

private:
  int64_t microsecond_time_;
};

}  // namespace base
#endif
