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

#include "line_codec_service.h"
#include "base/message_loop/message_loop.h"
#include "line_message.h"

#include <net_io/io_buffer.h>
#include <net_io/tcp_channel.h>
#include "glog/logging.h"

namespace lt {
namespace net {

LineCodecService::LineCodecService(base::MessageLoop* loop)
  : CodecService(loop) {}

LineCodecService::~LineCodecService() {}

void LineCodecService::OnDataFinishSend(const SocketChannel* channel) {
  ;
}

void LineCodecService::OnDataReceived(const SocketChannel*, IOBuffer* buf) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";
  const char* line_crlf = buf->FindCRLF();
  if (!line_crlf) {
    return;
  }

  {
    std::shared_ptr<LineMessage> msg(new LineMessage(InComingType()));
    msg->SetIOCtx(shared_from_this());

    const char* start = buf->GetRead();
    uint64_t len = line_crlf - start;

    std::string& body = msg->MutableBody();
    body.assign(start, len);

    buf->Consume(len + 2 /*lenth of /r/n*/);

    if (delegate_) {
      delegate_->OnCodecMessage(RefCast(CodecMessage, msg));
    }
  }
}

bool LineCodecService::SendMessage(CodecMessage* message) {
  static const std::string kCRCN("\r\n");
  const LineMessage* line_msg = static_cast<const LineMessage*>(message);
  int ret = channel_->Send(line_msg->Body().data(), line_msg->Body().size());
  if (ret < 0) {
    return false;
  }
  ret = channel_->Send(kCRCN.data(), 2);
  return ret >= 0;
}

bool LineCodecService::SendRequest(CodecMessage* req) {
  return SendMessage(req);
}

bool LineCodecService::SendResponse(const CodecMessage* req,
                                    CodecMessage* res) {
  return SendMessage(res);
};

}  // namespace net
}  // namespace lt
