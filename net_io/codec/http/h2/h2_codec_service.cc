
#include "h2_codec_service.h"

#include "base/utils/arrayutils.h"
#include "base/utils/string/str_utils.h"
#include "fmt/core.h"
#include "nghttp2_util.h"

using base::MessageLoop;

namespace lt {
namespace net {

namespace {

nghttp2_nv make_nv(const char* name, const char* value) {
  nghttp2_nv nv;
  nv.name = (uint8_t*)name;
  nv.value = (uint8_t*)value;
  nv.namelen = strlen(name);
  nv.valuelen = strlen(value);
  nv.flags = NGHTTP2_NV_FLAG_NONE;
  return nv;
}

nghttp2_nv make_nv(const std::string& name, const std::string& value) {
  nghttp2_nv nv;
  nv.name = (uint8_t*)name.data();
  nv.value = (uint8_t*)value.data();
  nv.namelen = name.size();
  nv.valuelen = value.size();
  nv.flags = NGHTTP2_NV_FLAG_NONE;
  return nv;
}

nghttp2_nv make_nv2(const char* name,
                    const char* value,
                    int namelen,
                    int valuelen) {
  nghttp2_nv nv;
  nv.name = (uint8_t*)name;
  nv.value = (uint8_t*)value;
  nv.namelen = namelen;
  nv.valuelen = valuelen;
  nv.flags = NGHTTP2_NV_FLAG_NONE;
  return nv;
}

template <size_t N>
nghttp2_nv make_nv_ls(const char (&name)[N], const std::string& value) {
  return {(uint8_t*)name,
          (uint8_t*)value.c_str(),
          N - 1,
          value.size(),
          NGHTTP2_NV_FLAG_NO_COPY_NAME};
}

int on_header(nghttp2_session* session,
              const nghttp2_frame* frame,
              const uint8_t* name,
              size_t namelen,
              const uint8_t* value,
              size_t valuelen,
              uint8_t flags,
              void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter";
  H2Util::PrintFrameHeader(&frame->hd);

  if (frame->hd.type != NGHTTP2_HEADERS ||
      frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    LOG(INFO) << __FUNCTION__ << " not NGHTTP2_HEADERS, or not a request";
    return 0;
  }

  H2CodecService* codec = (H2CodecService*)user_data;
  int32_t stream_id = frame->hd.stream_id;
  H2CodecService::StreamCtx* stream_ctx = codec->FindStreamCtx(stream_id);
  if (!stream_ctx) {
    return 0;
  }

  auto req = stream_ctx->Request();

  switch (lookup_token(name, namelen)) {
    case HD__METHOD:
      req->SetMethod(std::string(value, value + valuelen));
      break;
    case HD__SCHEME:
      // uref.scheme.assign(value, value + valuelen);
      req->InsertHeader("scheme", std::string(value, value + valuelen));
      break;
    case HD__AUTHORITY:
      req->InsertHeader("authority", std::string(value, value + valuelen));
      // uref.host.assign(value, value + valuelen);
      break;
    case HD__PATH:
      req->SetRequestURL(std::string(value, value + valuelen));
      break;
    case HD_HOST:
      req->InsertHeader("host", std::string(value, value + valuelen));
    // fall through
    default:
      // if (req.header_buffer_size() + namelen + valuelen > 64_k) {
      //  nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE,
      //  frame->hd.stream_id,
      //                            NGHTTP2_INTERNAL_ERROR);
      //  break;
      //}
      // req.update_header_buffer_size(namelen + valuelen);
      req->InsertHeader(std::string(name, name + namelen),
                        std::string(value, value + valuelen));
  }
  return 0;
}

int on_data_chunk(nghttp2_session* session,
                  uint8_t flags,
                  int32_t stream_id,
                  const uint8_t* data,
                  size_t len,
                  void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter";

  H2CodecService* codec = (H2CodecService*)user_data;
  H2CodecService::StreamCtx* stream_ctx = codec->FindStreamCtx(stream_id);
  if (!stream_ctx) {
    LOG(ERROR) << "stream:" << stream_id << " ctx not found";
    return 0;
  }

  if (codec->IsServerSide()) {
    auto req = stream_ctx->Request();
    req->AppendBody((const char*)data, len);
  } else {
    // TODO: response support
    // HttpRequest* req = stream_ctx->Request();
  }
  return 0;
}

// 网络io操作，将数据发送到网络
ssize_t send_callback(nghttp2_session* session,
                      const uint8_t* data,
                      size_t length,
                      int flags,
                      void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter, send data len:" << length
            << ", flag:" << flags;

  H2CodecService* codec = (H2CodecService*)user_data;

  auto channel = codec->Channel();
  if (channel->Send((const char*)data, length) >= 0) {
    return length;
  }
  return NGHTTP2_ERR_CALLBACK_FAILURE;
}

// 发送数据帧, 流程
// Send req/res->send HeaderFrame -> SendDataFrame -> BackTo
// SendRequest/SendResponse
int send_data_callback(nghttp2_session* session,
                       nghttp2_frame* frame,
                       const uint8_t* framehd,
                       size_t length,
                       nghttp2_data_source* source,
                       void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter";
  H2Util::PrintFrameHeader(&frame->hd);

  H2CodecService* conn = (H2CodecService*)user_data;
  H2CodecService::StreamCtx* stream_ctx =
      conn->FindStreamCtx(frame->hd.stream_id);
  // strm = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
  if (!stream_ctx) {
    return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
  }

  /* We never use padding in this program */
  assert(frame->data.padlen == 0);

  auto channel = conn->Channel();
  if (channel->Send((const char*)framehd, 9) < 0) {
    return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
  }
  const HttpResponse* response =
      (HttpResponse*)stream_ctx->Request()->RawResponse();
  CHECK(response);
  LOG(INFO) << __FUNCTION__ << ", enter send data body:" << response->Body();
  if (channel->Send(response->Body()) < 0) {
    return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
  }
  stream_ctx->data_frame_send_ = true;
  return 0;
}

static int on_begin_headers(nghttp2_session* session,
                            const nghttp2_frame* frame,
                            void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter";
  H2Util::PrintFrameHeader(&frame->hd);

  if (frame->hd.type != NGHTTP2_HEADERS ||
      frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    LOG(INFO) << __FUNCTION__ << ", not a request";
    return 0;
  }

  H2CodecService* codec = (H2CodecService*)user_data;

  int32_t stream_id = frame->hd.stream_id;
  codec->AddStreamCtx(stream_id);
  return 0;
}

static int on_stream_close(nghttp2_session* session,
                           int32_t stream_id,
                           uint32_t error_code,
                           void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter, stream:" << stream_id
            << ", error_code:" << error_code;
  (void)error_code;
  H2CodecService* codec = (H2CodecService*)user_data;
  codec->DelStreamCtx(stream_id);
  return 0;
}

static int on_frame_recv(nghttp2_session* session,
                         const nghttp2_frame* frame,
                         void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter";
  H2Util::PrintFrameHeader(&frame->hd);

  auto codec = static_cast<H2CodecService*>(user_data);
  H2CodecService::StreamCtx* stream_data =
      codec->FindStreamCtx(frame->hd.stream_id);
  if (!stream_data) {
    return 0;
  }

  switch (frame->hd.type) {
    case NGHTTP2_DATA:
      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
        // all body data recv, invoke request handler
        stream_data->Request()->SetIOCtx(codec->shared_from_this());
        codec->GetHandler()->OnCodecMessage(stream_data->Request());

        // stream_data->Request()->AppendBody();
        // strm->request().impl().call_on_data(nullptr, 0);
      }
      break;
    case NGHTTP2_HEADERS: {
      if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        break;
      }

      // auto& req = strm->request().impl();
      // req.remote_endpoint(handler->remote_endpoint());

      // all body data recv, invoke request handler
      stream_data->Request()->SetIOCtx(codec->shared_from_this());
      codec->GetHandler()->OnCodecMessage(stream_data->Request());

      if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
        // strm->request().impl().call_on_data(nullptr, 0);
      }
      break;
    }
  }
  return 0;
}

int on_frame_send(nghttp2_session* session,
                  const nghttp2_frame* frame,
                  void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter";
  H2Util::PrintFrameHeader(&frame->hd);

  if (frame->hd.type != NGHTTP2_PUSH_PROMISE) {
    return 0;
  }

  auto handler = static_cast<H2CodecService*>(user_data);

  H2CodecService::StreamCtx* stream_data =
      handler->FindStreamCtx(frame->hd.stream_id);
  if (!stream_data) {
    return 0;
  }
  // TODO: submit send response
  return 0;
}

int on_frame_not_send(nghttp2_session* session,
                      const nghttp2_frame* frame,
                      int lib_error_code,
                      void* user_data) {
  LOG(INFO) << __FUNCTION__ << ", enter, lib_error_code:" << lib_error_code;
  H2Util::PrintFrameHeader(&frame->hd);

  if (frame->hd.type != NGHTTP2_HEADERS) {
    return 0;
  }

  // Issue RST_STREAM so that stream does not hang around.
  nghttp2_submit_rst_stream(session,
                            NGHTTP2_FLAG_NONE,
                            frame->hd.stream_id,
                            NGHTTP2_INTERNAL_ERROR);

  return 0;
}

nghttp2_session_callbacks* new_nghttp2_callback() {
#define cb_set(name, cbs, func) nghttp2_session_callbacks_set_##name(cbs, func)

  nghttp2_session_callbacks* cbs;
  nghttp2_session_callbacks_new(&cbs);

  // write data handle callback
  // 发送数据
  cb_set(send_callback, cbs, send_callback);
  // 发送数据帧, 为了减少copy
  cb_set(send_data_callback, cbs, send_data_callback);
  cb_set(on_frame_send_callback, cbs, on_frame_send);
  cb_set(on_frame_not_send_callback, cbs, on_frame_not_send);

  // read data handle callback
  //
  // 代表一个stream request开始，创建对应的streamctx并完成出事化
  // 后续通过nghttp2_session_get_stream_user_data/codec.FindStreamCtx
  // 拿到对应stream id的requet
  cb_set(on_begin_headers_callback, cbs, on_begin_headers);
  cb_set(on_header_callback, cbs, on_header);
  cb_set(on_frame_recv_callback, cbs, on_frame_recv);
  cb_set(on_data_chunk_recv_callback, cbs, on_data_chunk);

  cb_set(on_stream_close_callback, cbs, on_stream_close);

#undef cb_set
  return cbs;
}

nghttp2_session_callbacks* cbs_ = new_nghttp2_callback();

}  // namespace

H2CodecService::H2CodecService(MessageLoop* loop) : CodecService(loop) {}

H2CodecService::~H2CodecService() {
  VLOG(VTRACE) << __FUNCTION__ << ", this@" << this << " Gone";
  if (session_) {
    nghttp2_session_del(session_);
  }
  stream_ctxs_.clear();
}

void H2CodecService::StartProtocolService() {
  if (IsServerSide()) {
    nghttp2_session_server_new(&session_, cbs_, this);
  } else {
    nghttp2_session_client_new(&session_, cbs_, this);
  }
  nghttp2_settings_entry settings[] = {
      //{NGHTTP2_SETTINGS_ENABLE_PUSH, 1},
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

  CodecService::StartProtocolService();

  nghttp2_submit_settings(session_,
                          NGHTTP2_FLAG_NONE,
                          settings,
                          ARRAY_SIZE(settings));

  nghttp2_session_send(session_);
}

void H2CodecService::OnDataReceived(IOBuffer* buf) {
  LOG(INFO) << __FUNCTION__ << ", enter";

  const uint8_t* data = buf->GetReadU();
  const size_t len = buf->CanReadSize();
  size_t ret = nghttp2_session_mem_recv(session_, data, len);
  if (ret != buf->CanReadSize()) {
    LOG(ERROR) << __FUNCTION__ << " nghttp2 mem recv failed";
    return CloseService(false);
  }
  buf->Consume(len);
}

void H2CodecService::HandleEvent(base::FdEvent* fdev, base::LtEv::Event ev) {
  VLOG(VTRACE) << __FUNCTION__ << ", enter";

  CodecService::HandleEvent(fdev, ev);
  if (IsClosed()) {
    return;
  }

  do {
    // write stream buffer
    if (0 != nghttp2_session_send(session_)) {
      break;
    }
    if (IsSessionClosed() || ShouldClose()) {
      break;
    }

    return; // end flow success
  }while(0);

  return CloseService(false);
}

bool H2CodecService::IsSessionClosed() const {
  if (!session_) {
    return false;
  }
  return !nghttp2_session_want_read(session_) &&
         !nghttp2_session_want_write(session_);
}

void H2CodecService::PushPromise(const std::string& method,
                                 const std::string& req_path,
                                 const HttpRequest* bind_req,
                                 const RefHttpResponse& resp,
                                 std::function<void(int status)> callback) {
#define DO_NOTIFY_CALLBACK(x) \
  if (callback) {             \
    callback(x);              \
  }

  int32_t stream_id = bind_req->StreamID();
  StreamCtx* ctx = FindStreamCtx(stream_id);
  if (!ctx) {
    DO_NOTIFY_CALLBACK(-1);  // bind stream not found
    return;
  }

  auto nva = std::vector<nghttp2_nv>();
  nva.reserve(4);
  nva.push_back(make_nv_ls(":method", method));
  nva.push_back(make_nv_ls(":scheme", bind_req->GetHeader("scheme")));
  nva.push_back(make_nv_ls(":authority", bind_req->GetHeader("authority")));
  nva.push_back(make_nv_ls(":path", req_path));

  /*
  for (auto &hd : h) {
    nva.push_back(nghttp2::http2::make_nv(hd.first, hd.second.value,
                                          hd.second.sensitive));
  }*/

  int promise_id = nghttp2_submit_push_promise(session_,
                                               NGHTTP2_FLAG_NONE,
                                               stream_id,
                                               nva.data(),
                                               nva.size(),
                                               nullptr);
  if (promise_id < 0) {
    DO_NOTIFY_CALLBACK(-2);
    return;
  }
  StreamCtx* promise_ctx = AddStreamCtx(promise_id);
  HttpRequest* promise_req = promise_ctx->Request().get();
  promise_req->SetMethod(method);
  promise_req->SetRequestURL(req_path);

  resp->SetStreamID(promise_id);
  promise_req->SetResponse(resp);
  bool ok = SendResponse(promise_req, resp.get());
  DO_NOTIFY_CALLBACK((ok ? 0 : -3));
#undef DO_NOTIFY_CALLBACK
  return;
}

void H2CodecService::BeforeSendRequest(HttpRequest*) {
  LOG(INFO) << __FUNCTION__ << "enter";
}

bool H2CodecService::SendRequest(CodecMessage* message) {
  LOG(INFO) << __FUNCTION__ << "enter";
  return true;
}

bool H2CodecService::BeforeSendResponse(const HttpRequest*, HttpResponse*) {
  LOG(INFO) << __FUNCTION__ << "enter";

  return true;
}

bool H2CodecService::SendResponse(const CodecMessage* req, CodecMessage* r) {
  LOG(INFO) << __FUNCTION__ << ", enter, res:" << r->Dump();
  HttpResponse* res = (HttpResponse*)(r);

  std::vector<nghttp2_nv> nvs;

  std::string status = fmt::format("{}", res->ResponseCode());
  nvs.push_back(make_nv(":status", status.data()));
  for (auto& kv : res->Headers()) {
    // HTTP2 default keep-alive
    if (base::StrUtil::IgnoreCaseEquals(kv.first, "connection")) {
      continue;
    }
    // HTTP2 have frame_hd.length
    if (base::StrUtil::IgnoreCaseEquals(kv.first, "content-length")) {
      continue;
    }
    nvs.push_back(make_nv(kv.first, kv.second));
  }

  StreamCtx* ctx = FindStreamCtx(req->AsyncId());

  nghttp2_data_provider *prd_ptr = nullptr, prd;
  if (res->Body().size() > 0) {
    prd.source.ptr = ctx;
    prd.read_callback = [](nghttp2_session* session,
                           int32_t stream_id,
                           uint8_t* buf,
                           size_t length,
                           uint32_t* data_flags,
                           nghttp2_data_source* source,
                           void* user_data) -> ssize_t {
      LOG(INFO) << "data provider:" << length;
      *data_flags |= NGHTTP2_DATA_FLAG_NO_COPY;
      *data_flags |= NGHTTP2_DATA_FLAG_EOF;

      StreamCtx* _ctx = (StreamCtx*)source->ptr;
      if (!_ctx || _ctx->data_frame_send_) {
        return 0;
      }
      HttpResponse* res = (HttpResponse*)(_ctx->Request()->RawResponse());
      return res->Body().size();
    };
    prd_ptr = &prd;
  }

  int rv = nghttp2_submit_response(session_,
                                   req->AsyncId(),
                                   nvs.data(),
                                   nvs.size(),
                                   prd_ptr);
  LOG(INFO) << "submit response return value:" << rv;
  // nghttp2_submit_headers(session_, flags, stream_id, NULL, &nvs[0],
  // nvs.size(), NULL);
  // avoid DATA_SOURCE_COPY, we do not use nghttp2_submit_data
  // data_prd.read_callback = data_source_read_callback;
  // nghttp2_submit_response(session, stream_id, &nvs[0], nvs.size(),
  // &data_prd);
  return rv == 0 && 0 == nghttp2_session_send(session_);
}

const RefCodecMessage H2CodecService::NewResponse(const CodecMessage*) {
  return HttpResponse::CreateWithCode(200);
}

bool H2CodecService::UseSSLChannel() const {
  return protocol_ == "h2";
}

H2CodecService::StreamCtx* H2CodecService::AddStreamCtx(int32_t sid) {
  auto ret = stream_ctxs_.emplace(sid, StreamCtx(IsServerSide(), sid));
  if (!ret.second) {
    return nullptr;
  }
  LOG(INFO) << __FUNCTION__
            << " add streamctx, id:" << ret.first->second.Request()->AsyncId();
  return &(ret.first->second);
}

H2CodecService::StreamCtx* H2CodecService::FindStreamCtx(int32_t sid) {
  auto iter = stream_ctxs_.find(sid);
  if (iter == stream_ctxs_.end()) {
    return nullptr;
  }
  return &(iter->second);
}

void H2CodecService::DelStreamCtx(int32_t sid) {
  stream_ctxs_.erase(sid);
}

}  // namespace net
}  // namespace lt
