#include "glog/logging.h"
#include "io_buffer.h"
#include "tcp_channel.h"
#include "line_message.h"
#include "line_proto_service.h"

namespace net {

LineProtoService::LineProtoService()
  : ProtoService() {
}

LineProtoService::~LineProtoService() {
}

void LineProtoService::OnStatusChanged(const RefTcpChannel& channel) {
  ;
}
void LineProtoService::OnDataFinishSend(const RefTcpChannel& channel) {
  ;
}

void LineProtoService::OnDataReceived(const RefTcpChannel&, IOBuffer *buf) {
  const uint8_t* line_crlf =  buf->FindCRLF();
  if (!line_crlf) {
    return;
  }

  {
    std::shared_ptr<LineMessage> msg(new LineMessage(InComingType()));
    msg->SetIOContext(shared_from_this());

    const uint8_t* start = buf->GetRead();
    uint64_t len = line_crlf - start;

    std::string& body = msg->MutableBody();
    body.assign((const char*)start, len);

    buf->Consume(len + 2/*lenth of /r/n*/);

    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(msg));
    }
  }
}

bool LineProtoService::SendRequestMessage(const RefProtocolMessage &message) {
  static const std::string kCRCN("\r\n");
  const LineMessage* line_msg = static_cast<const LineMessage*>(message.get());
  int ret = channel_->Send((const uint8_t*)line_msg->Body().data(), line_msg->Body().size());
  if (ret < 0) {
    return false;
  }
  ret = channel_->Send((const uint8_t*)kCRCN.data(), 2);
  return ret >= 0;
}

bool LineProtoService::ReplyRequest(const RefProtocolMessage& req, const RefProtocolMessage& res) {
  return SendRequestMessage(res);
};

bool LineProtoService::CloseAfterMessage(ProtocolMessage*, ProtocolMessage*) {
  return false;
}

}//end of file
