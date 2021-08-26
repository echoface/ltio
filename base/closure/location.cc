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

#include "location.h"

#include <base/logging.h>
#include "fmt/format.h"

namespace base {

Location::Location(const Location& other) = default;

Location::Location(const char* function, const char* file, int line)
  : line_number_(line),
    file_name_(file),
    function_name_(function) {}

std::string Location::ToString() const {
  if (!has_source_info()) {
    return "";
  }
  return fmt::format("{}@{}:{}", function_name_, file_name_, line_number_);
}

#if defined(__clang__)
#define RETURN_ADDRESS() nullptr
#elif defined(__GNUC__) || defined(__GNUG__)
#define RETURN_ADDRESS() \
  __builtin_extract_return_addr(__builtin_return_address(0))
#elif defined(_MSC_VER)
#define RETURN_ADDRESS() _ReturnAddress()
#else
#define RETURN_ADDRESS() nullptr
#endif

}  // namespace base
