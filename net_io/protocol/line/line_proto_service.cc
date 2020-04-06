#include "base/message_loop/message_loop.h"
#include "line_message.h"
#include "line_proto_service.h"

#include "glog/logging.h"
#include <net_io/io_buffer.h>
#include <net_io/tcp_channel.h>

namespace lt {
namespace net {

LineProtoService::LineProtoService(base::MessageLoop* loop)
  : ProtoService(loop) {
}

LineProtoService::~LineProtoService() {
}

void LineProtoService::OnStatusChanged(const SocketChannel* channel) {
  ;
}
void LineProtoService::OnDataFinishSend(const SocketChannel* channel) {
  ;
}

void LineProtoService::OnDataReceived(const SocketChannel*, IOBuffer *buf) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";
  const char* line_crlf =  buf->FindCRLF();
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

    buf->Consume(len + 2/*lenth of /r/n*/);

    if (delegate_) {
      delegate_->OnProtocolMessage(std::static_pointer_cast<ProtocolMessage>(msg));
    }
  }
}

bool LineProtoService::EncodeToChannel(ProtocolMessage* message) {
  static const std::string kCRCN("\r\n");
  const LineMessage* line_msg = static_cast<const LineMessage*>(message);
  int ret = channel_->Send(line_msg->Body().data(), line_msg->Body().size());
  if (ret < 0) {
    return false;
  }
  ret = channel_->Send(kCRCN.data(), 2);
  return ret >= 0;
}

bool LineProtoService::EncodeResponseToChannel(const ProtocolMessage* req, ProtocolMessage* res) {
  return EncodeToChannel(res);
};

}}//end of file
