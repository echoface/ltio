#include "ws_codec_service.h"

#include "fmt/core.h"
#include "hash/murmurhash3.h"

#include "base/utils/base64/base64.hpp"
#include "base/utils/rand_util.h"
#include "net_io/codec/http/http_codec_service.h"
#include "ws_util.h"

namespace lt {
namespace net {

namespace {
const char* upgrade_response_template =
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Connection: Upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Accept: {}\r\n\r\n";

constexpr size_t MAX_PAYLOAD_LENGTH = (1 << 24);  // 16M

WebsocketFrame* kclose_msg = WebsocketFrame::NewCloseFrame(1000);
WebsocketFrame kping_msg(WS_OPCODE_PING);
WebsocketFrame kpong_msg(WS_OPCODE_PONG);

};  // namespace

WSCodecService::WSCodecService(base::MessageLoop* loop) : CodecService(loop) {
  websocket_parser_init(&parser_);
  parser_.data = this;

  websocket_parser_settings_init(&settings_);
  settings_.on_frame_header = OnFrameBegin;
  settings_.on_frame_body = OnFrameBody;
  settings_.on_frame_end = OnFrameFinish;
}

WSCodecService::~WSCodecService() {
  if (http_res_parser()) {
    finalize_http_parser();
  }
}

void WSCodecService::init_http_parser() {
  if (IsServerSide()) {
    http_parser_.req_parser = new HttpReqParser(this);
  } else {
    http_parser_.res_parser = new HttpResParser(this);
  }
}

void WSCodecService::finalize_http_parser() {
  if (IsServerSide()) {
    delete http_req_parser();
    http_parser_.req_parser = nullptr;
  } else {
    delete http_res_parser();
    http_parser_.res_parser = nullptr;
  }
}

// static
int WSCodecService::OnFrameBegin(websocket_parser* parser) {
  auto codec = (WSCodecService*)parser->data;
  if (!codec->current_) {
    codec->current_ = std::make_shared<WebsocketFrame>();
  }
  WebsocketFrame* message = codec->current_.get();
  int opcode = parser->flags & WS_OP_MASK;
  if (opcode != WS_OP_CONTINUE) {
    message->SetOpCode(opcode);
  }
  /*
  size_t length = parser->length;
  int reserve_length = std::min(length + 1, MAX_PAYLOAD_LENGTH);
  if (reserve_length > wp->message.capacity()) {
    wp->message.reserve(reserve_length);
  }*/
  return 0;
}

int WSCodecService::OnFrameBody(websocket_parser* parser,
                                const char* data,
                                size_t len) {
  auto* codec = (WSCodecService*)parser->data;
  if (parser->flags & WS_HAS_MASK) {
    websocket_parser_decode((char*)data, data, len, parser);
  }
  VLOG(GLOG_VTRACE) << "ws parse body:" << std::string(data, len);
  codec->current_->AppendData(data, len);
  return 0;
}

int WSCodecService::OnFrameFinish(websocket_parser* parser) {
  auto* codec = (WSCodecService*)parser->data;
  if (parser->flags & WS_FIN) {
    codec->CommitFrameMessage();
  }
  return 0;
}

void WSCodecService::CommitFrameMessage() {
  switch (current_->OpCode()) {
    case WS_OPCODE_CLOSE:
      LOG(INFO) << "CloseFrame:" << current_->Dump();
      SendResponse(nullptr, kclose_msg);
      CloseService(true);
      return delegate_->OnCodecClosed(shared_from_this());
      break;
    case WS_OPCODE_PING:
      SendResponse(nullptr, &kpong_msg);
      break;
    default:
      break;
  }
  current_->SetIOCtx(shared_from_this());
  handler_->OnCodecMessage(std::move(current_));
}

void WSCodecService::StartProtocolService() {
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";

  StartInternal();

  init_http_parser();

  /* handshake upgrade request
  GET / HTTP/1.1
  Host: localhost:8080
  Origin: http://127.0.0.1:3000
  Connection: Upgrade
  Upgrade: websocket
  Sec-WebSocket-Version: 13
  Sec-WebSocket-Key: w4v7O6xFTi36lq3RNcgctw==
  */
  if (!IsServerSide()) {
    // send a handshake http upgrade request
    uint64_t key = base::RandUint64();
    std::array<uint8_t, 16> out;
    MurmurHash3_x64_128(&key, sizeof(key), 0xf03c, out.data());
    std::string sec_key = base::base64_encode(out.data(), out.size());

    auto hs_req = std::make_shared<HttpRequest>();
    hs_req->SetMethod("GET");
    hs_req->SetRequestURL(ws_path_);
    const url::RemoteInfo* info = delegate_->GetRemoteInfo();
    hs_req->InsertHeader("Host", info->host);
    hs_req->InsertHeader("Upgrade", "websocket");
    hs_req->InsertHeader("Connection", "Upgrade");
    hs_req->InsertHeader("Sec-WebSocket-Version", "13");
    hs_req->InsertHeader("Sec-WebSocket-Key", sec_key);
    HttpCodecService::RequestToBuffer(hs_req.get(), channel_->WriterBuffer());
    VLOG(GLOG_VTRACE) << "send handshake request"
                      << channel_->WriterBuffer()->AsString();
    ignore_result(channel_->TryFlush());
  }
}

bool WSCodecService::Send(RefWebsocketFrame message) {
  CodecMessage* ptr = message.get();
  return IsServerSide() ? SendResponse(nullptr, ptr) : SendRequest(ptr);
}

bool WSCodecService::SendRequest(CodecMessage* req) {
  WebsocketFrame* wsmsg = (WebsocketFrame*)req;
  size_t size =
      WebsocketUtil::CalculateFrameSize(wsmsg->Payload().size(), true);

  auto buffer = channel_->WriterBuffer();
  buffer->EnsureWritableSize(size);
  char* start = buffer->GetWrite();
  int n = WebsocketUtil::BuildClientFrame(start,
                                          wsmsg->Payload().data(),
                                          wsmsg->Payload().size(),
                                          (ws_opcode)wsmsg->OpCode());
  buffer->Produce(size);
  VLOG(GLOG_VTRACE) << "send request size:" << size << ", n:" << n
                    << " content:" << wsmsg->Payload();
  return channel_->TryFlush();
}

bool WSCodecService::SendResponse(const CodecMessage*, CodecMessage* res) {
  WebsocketFrame* wsmsg = (WebsocketFrame*)res;
  size_t size = WebsocketUtil::CalculateFrameSize(wsmsg->Payload().size());

  auto buffer = channel_->WriterBuffer();
  buffer->EnsureWritableSize(size);
  char* start = buffer->GetWrite();

  int n = WebsocketUtil::BuildServerFrame(start,
                                          wsmsg->Payload().data(),
                                          wsmsg->Payload().size(),
                                          (ws_opcode)wsmsg->OpCode());
  buffer->Produce(n);
  VLOG(GLOG_VTRACE) << "send response size:" << size << ", n:" << n
                    << " payload:" << wsmsg->Payload();

  return channel_->TryFlush();
}

/*
 * HTTP/1.1 101 Switching Protocols
 *
 * Connection: Upgrade
 * Upgrade: websocket
 * Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
 */

void WSCodecService::CommitHttpRequest(const RefHttpRequest&& request) {
  CHECK(IsServerSide());
  if (request->GetHeader("Upgrade") != "websocket") {
    hs_state = handshake_state::HS_WRONGMSG;
    return;
  }

  ws_path_ = request->Path().to_string();
  sec_key_ = request->GetHeader(SEC_WEBSOCKET_KEY);

  auto ws_accept = WebsocketUtil::GenAcceptKey(sec_key_.c_str());

  std::string response = fmt::format(upgrade_response_template, ws_accept);
  VLOG(GLOG_VINFO) << "websocket upgrade, path:" << ws_path_
                   << ",key:" << sec_key_ << ",rsp:" << response;

  if (channel_->Send(response) < 0) {
    hs_state = handshake_state::HS_WRONGMSG;
    return;
  }
  hs_state = handshake_state::HS_SUCCEESS;
  NotifyCodecReady();
}

// OnDataReceived -> OnHandshakeReqData -> CommitHttpRequest -> OnChannelReady
void WSCodecService::OnHandshakeReqData(IOBuffer* buf) {
  CHECK(IsServerSide());

  auto parser = http_req_parser();

  if (HttpReqParser::Error == parser->Parse(buf)) {
    LOG(ERROR) << "parse handshake request error";

    ignore_result(channel_->Send(HttpConstant::kBadRequest));
    CloseService(true);
    return delegate_->OnCodecClosed(shared_from_this());
  }

  if (hs_state == HS_SUCCEESS) {
    return NotifyCodecReady();
  }

  if (hs_state == HS_WRONGMSG) {
    LOG(ERROR) << "got bad handshake message";
    ignore_result(channel_->Send(HttpConstant::kBadRequest));
    CloseService(true);
    return NotifyCodecClosed();
  }
}

void WSCodecService::CommitHttpResponse(const RefHttpResponse&& response) {
  CHECK(!IsServerSide());

  sec_accept_ = response->GetHeader(SEC_WEBSOCKET_ACCEPT);
  if (sec_accept_.size() > 0 && http_res_parser()->Upgrade()) {
    hs_state = handshake_state::HS_SUCCEESS;
  }
}

void WSCodecService::OnHandshakeResData(IOBuffer* buf) {
  CHECK(!IsServerSide());

  auto parser = http_res_parser();
  if (HttpResParser::Error == parser->Parse(buf)) {
    LOG(ERROR) << "parse handshake response error";
    ignore_result(channel_->Send(HttpConstant::kBadRequest));
    CloseService(true);
    return NotifyCodecClosed();
  }

  if (hs_state == HS_SUCCEESS) {
    return NotifyCodecReady();
  }

  if (hs_state == HS_WRONGMSG) {
    LOG(ERROR) << "got bad handshake message";
    ignore_result(channel_->Send(HttpConstant::kBadRequest));
    CloseService(true);
    return NotifyCodecClosed();
  }
}

void WSCodecService::OnDataReceived(IOBuffer* buf) {
  if (hs_state != HS_SUCCEESS) {
    return IsServerSide() ? OnHandshakeReqData(buf) : OnHandshakeResData(buf);
  }

  auto guard = shared_from_this();

  size_t nparsed = websocket_parser_execute(&parser_,
                                            &settings_,
                                            buf->GetRead(),
                                            buf->CanReadSize());
  if (nparsed != buf->CanReadSize()) {
    CloseService(true);
    return NotifyCodecClosed();
  }

  buf->Consume(nparsed);
}

}  // namespace net
}  // namespace lt
