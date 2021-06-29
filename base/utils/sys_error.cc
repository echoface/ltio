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

#include "sys_error.h"
#include <string.h>
#include <vector>

#include "base/build_config.h"

#if HAVE_ERRNO_H
#include <errno.h>
#endif

namespace base {

std::string StrError() {
  return StrError(errno);
}

std::string StrError(int errnum) {
  std::string error_str;
  if (errnum == 0)
    return error_str;

  const int MaxErrStrLen = 256;
  static std::vector<char> buffer(MaxErrStrLen);

  buffer[0] = '\0';

// posix
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || \
                         (defined(__APPLE__) && defined(__MACH__)))

#if defined(__GLIBC__)
  // glibc defines its own incompatible version of strerror_r
  // which may not use the buffer supplied.
  error_str = strerror_r(errnum, &buffer[0], MaxErrStrLen - 1);
#else
  strerror_r(errnum, &buffer[0], MaxErrStrLen - 1);
  error_str = (char*)buffer[0];
#endif

#else
  error_str = strerror(errnum);
#endif

  return error_str;
}  // namespace base

}  // namespace base
