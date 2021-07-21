#include "ws_codec_service.h"

#include "fmt/core.h"
#include "ws_util.h"

namespace lt {
namespace net {

namespace {
const char* access_response_template =
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Connection: Upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Accept: {}\r\n\r\n";

constexpr size_t MAX_PAYLOAD_LENGTH = (1 << 24);  // 16M

WebsocketMessage kpong_res(MessageType::kResponse, WS_OPCODE_PONG);
WebsocketMessage kclose_res(MessageType::kResponse, WS_OPCODE_CLOSE);

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
    auto type = codec->RecievedMessageType();
    codec->current_ = std::make_shared<WebsocketMessage>(type);
  }
  WebsocketMessage* message = codec->current_.get();
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
      SendResponse(nullptr, &kclose_res);

      CloseService(true);
      return delegate_->OnCodecClosed(shared_from_this());
      break;
    case WS_OPCODE_PING:
      SendResponse(nullptr, &kpong_res);
      break;
    default:
      break;
  }
  current_->SetIOCtx(shared_from_this());
  handler_->OnCodecMessage(std::move(current_));
}

void WSCodecService::StartProtocolService() {
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";

  init_http_parser();

  CodecService::StartProtocolService();
}

bool WSCodecService::Send(RefWebsockMessage message) {
  CodecMessage* ptr = message.get();
  return IsServerSide() ? SendResponse(nullptr, ptr) : SendRequest(ptr);
}

bool WSCodecService::SendRequest(CodecMessage* req) {
  WebsocketMessage* wsmsg = (WebsocketMessage*)req;
  size_t size =
      WebsocketUtil::CalculateFrameSize(wsmsg->Payload().size(), true);

  auto buffer = channel_->WriterBuffer();
  buffer->EnsureWritableSize(size);
  char* start = buffer->GetWrite();
  int n = WebsocketUtil::BuildClientFrame(start,
                                          wsmsg->Payload().data(),
                                          wsmsg->Payload().size(),
                                          (ws_opcode)wsmsg->OpCode());
  buffer->Produce(n);

  return channel_->TryFlush();
}

bool WSCodecService::SendResponse(const CodecMessage*, CodecMessage* res) {
  WebsocketMessage* wsmsg = (WebsocketMessage*)res;
  size_t size = WebsocketUtil::CalculateFrameSize(wsmsg->Payload().size());

  auto buffer = channel_->WriterBuffer();
  buffer->EnsureWritableSize(size);
  char* start = buffer->GetWrite();

  int n = WebsocketUtil::BuildServerFrame(start,
                                          wsmsg->Payload().data(),
                                          wsmsg->Payload().size(),
                                          (ws_opcode)wsmsg->OpCode());
  buffer->Produce(n);

  return channel_->TryFlush();
}

/*
 * GET / HTTP/1.1
 *
 * Host: localhost:8080
 * Origin: http://127.0.0.1:3000
 * Connection: Upgrade
 * Upgrade: websocket
 * Sec-WebSocket-Version: 13
 * Sec-WebSocket-Key: IRhw449z7G0Mov9CahJ+Ow==
 * */
void WSCodecService::OnChannelReady(const SocketChannel* ch) {
  CHECK(delegate_);
  VLOG(GLOG_VINFO) << __FUNCTION__ << ", ch:" << ch->ChannelInfo();

  // Issue a handshake request
  // auto hs_req = std::make_shared<HttpRequest>();
  return;
}

void WSCodecService::OnChannelClosed(const SocketChannel*) {}

void WSCodecService::OnDataFinishSend(const SocketChannel*) {}

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

  std::string response = fmt::format(access_response_template, ws_accept);
  VLOG(GLOG_VINFO) << "websocket upgrade, path:" << ws_path_
                   << ",key:" << sec_key_ << ",rsp:" << response;

  int32_t ret = channel_->Send(response.data(), response.size());
  if (ret < 0) {
    hs_state = handshake_state::HS_WRONGMSG;
    return;
  }
  hs_state = handshake_state::HS_SUCCEESS;
  CodecService::OnChannelReady(channel_.get());
}

// OnDataReceived -> OnHandshakeReqData -> CommitHttpRequest -> OnChannelReady
void WSCodecService::OnHandshakeReqData(IOBuffer* buf) {
  CHECK(IsServerSide());

  auto parser = http_req_parser();

  if (HttpReqParser::Error == parser->Parse(buf)) {
    LOG(ERROR) << "parse handshake request error";

    channel_->Send(HttpConstant::kBadRequest.data(),
                   HttpConstant::kBadRequest.size());

    CloseService(true);

    return delegate_->OnCodecClosed(shared_from_this());
  }

  if (hs_state == HS_SUCCEESS) {
    return CodecService::OnChannelReady(channel_.get());
  }

  if (hs_state == HS_WRONGMSG) {
    LOG(ERROR) << "got bad handshake message";
    channel_->Send(HttpConstant::kBadRequest.data(),
                   HttpConstant::kBadRequest.size());

    CloseService(true);
    return delegate_->OnCodecClosed(shared_from_this());
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
    channel_->Send(HttpConstant::kBadRequest.data(),
                   HttpConstant::kBadRequest.size());

    CloseService(true);
    return delegate_->OnCodecClosed(shared_from_this());
  }

  if (hs_state == HS_SUCCEESS) {
    return CodecService::OnChannelReady(channel_.get());
  }

  if (hs_state == HS_WRONGMSG) {
    LOG(ERROR) << "got bad handshake message";
    channel_->Send(HttpConstant::kBadRequest.data(),
                   HttpConstant::kBadRequest.size());

    CloseService(true);
    return delegate_->OnCodecClosed(shared_from_this());
  }
}

void WSCodecService::OnDataReceived(const SocketChannel*, IOBuffer* buf) {
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
    return delegate_->OnCodecClosed(guard);
  }

  buf->Consume(nparsed);
}

}  // namespace net
}  // namespace lt
