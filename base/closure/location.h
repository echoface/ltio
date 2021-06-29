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

#ifndef BASE_LOCATION_H_H_
#define BASE_LOCATION_H_H_

#include <string>

namespace base {

class Location {
public:
  // Constructor should be called with a long-lived char*, such as __FILE__.
  // It assumes the provided value will persist as a global constant, and it
  // will not make a copy of it.
  Location(){};
  Location(const Location& other);
  Location(const char* function_name, const char* file_name, int line_number);

  // Returns true if there is source code location info. If this is false,
  // the Location object only contains a program counter or is
  // default-initialized (the program counter is also null).
  bool has_source_info() const { return function_name_ && file_name_; }

  // Will be nullptr for default initialized Location objects and when source
  // names are disabled.
  const char* function_name() const { return function_name_; }

  // Will be nullptr for default initialized Location objects and when source
  // names are disabled.
  const char* file_name() const { return file_name_; }

  // Will be -1 for default initialized Location objects and when source names
  // are disabled.
  int line_number() const { return line_number_; }

  // Converts to the most user-readable form possible. If function and filename
  // are not available, this will return "pc:<hex address>".
  std::string ToString() const;

private:
  int line_number_ = -1;
  const char* file_name_ = nullptr;
  const char* function_name_ = nullptr;
};

#define FROM_HERE ::base::Location(__func__, __FILE__, __LINE__)

}  // namespace base

#endif
