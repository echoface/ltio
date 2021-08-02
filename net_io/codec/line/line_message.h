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

#ifndef _NET_LINE_PROTO_MESSAGE_H
#define _NET_LINE_PROTO_MESSAGE_H

#include <net_io/codec/codec_message.h>
#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

class LineMessage : public CodecMessage {
public:
  typedef LineMessage ResponseType;
  LineMessage();
  LineMessage(const std::string& data);

  ~LineMessage();

  std::string& MutableBody();

  const std::string& Body() const;

private:
  std::string content_;
};

}  // namespace net
}  // namespace lt
#endif
