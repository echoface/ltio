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

#ifndef BASE_REFCOUNTEDOBJECT_H_
#define BASE_REFCOUNTEDOBJECT_H_

#include <utility>

namespace base {

template <class T>
class RefCountedObject : public T {
public:
  RefCountedObject() {}

  template <class P0>
  explicit RefCountedObject(P0&& p0) : T(std::forward<P0>(p0)) {}

  template <class P0, class P1, class... Args>
  RefCountedObject(P0&& p0, P1&& p1, Args&&... args)
    : T(std::forward<P0>(p0),
        std::forward<P1>(p1),
        std::forward<Args>(args)...) {}

  virtual int AddRef() const { return __sync_add_and_fetch(&ref_count_, 1); }

  virtual int Release() const {
    int count = __sync_sub_and_fetch(&ref_count_, 1);
    if (!count) {
      delete this;
    }
    return count;
  }

  virtual bool HasOneRef() const {
    return 1 == __atomic_load_n(&ref_count_, __ATOMIC_ACQUIRE);
  }

protected:
  virtual ~RefCountedObject() {}
  mutable volatile int ref_count_ = 0;
};

}  // namespace base
#endif
