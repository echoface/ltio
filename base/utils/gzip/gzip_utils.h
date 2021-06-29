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

#include <sstream>
#include <stdexcept>
#include <string>

#include <string.h>
#include "zlib.h"

namespace base {

inline char* str_as_array(std::string* str) {
  return str->empty() ? NULL : &*str->begin();
}
inline const char* str_as_array(const std::string& str) {
  return str_as_array(const_cast<std::string*>(&str));
}

namespace Gzip {

// return zero when success, other for fails
int decompress_gzip(const std::string& str, std::string& out);
int decompress_deflate(const std::string& str, std::string& out);

//
int compress_gzip(const std::string& str,
                  std::string& out,
                  int compressionlevel = Z_BEST_COMPRESSION);
// std::string compress_gzip(const std::string& str, int compressionlevel =
// Z_BEST_COMPRESSION);
int compress_deflate(const std::string& str,
                     std::string& out,
                     int compressionlevel = Z_BEST_COMPRESSION);

}  // namespace Gzip
}  // namespace base
