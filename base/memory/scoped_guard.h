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

#ifndef LIGHTINGIO_SCOPED_GUARD_H
#define LIGHTINGIO_SCOPED_GUARD_H

#include <functional>

namespace base {

class ScopedGuard {
public:
  ScopedGuard(std::function<void()> func) : func_(func) {}
  ~ScopedGuard() {
    if (!dismiss_ && func_) {
      func_();
    }
  }
  void DisMiss() { dismiss_ = true; }

private:
  bool dismiss_ = false;
  std::function<void()> func_;
};

}  // namespace base
#endif  // LIGHTINGIO_SCOPED_GUARD_H
