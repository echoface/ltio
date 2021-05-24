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

#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <memory>
#include <functional>

#include <base/base_micro.h>


#if not defined(COROUTINE_STACK_SIZE)
#define COROUTINE_STACK_SIZE 262144 // 32768 * sizeof(void*)
#endif

namespace base {

enum class CoroState {
  kInit    = 0,
  kRunning = 1,
  kPaused  = 2,
  kDone    = 3
};

int64_t SystemCoroutineCount();
std::string StateToString(CoroState st);

class Coroutine;
typedef std::weak_ptr<Coroutine> WeakCoroutine;
typedef std::shared_ptr<Coroutine> RefCoroutine;

} //end if base
#endif
