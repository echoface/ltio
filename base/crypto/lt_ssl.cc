#include "lt_ssl.h"

#include <string.h>
#include <mutex>

#include "base/memory/lazy_instance.h"
#include "glog/logging.h"

namespace base {

namespace {

std::once_flag lib_init_flag;

void print_openssl_err() {
  int err;
  char err_str[256] = {0};
  while ((err = ERR_get_error())) {
    ERR_error_string_n(err, &err_str[0], 256);
    LOG(ERROR) << "ssl err:" << err_str;
  }
}

void open_ssl_lib_init() {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
  CONF_modules_load(NULL, NULL, 0);
#elif OPENSSL_VERSION_NUMBER >= 0x0907000L
  OPENSSL_config(NULL);
#endif
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  print_openssl_err();
}

template <SSLRole role>
struct DefaultCtxWrapper : public SSLCtx {
  DefaultCtxWrapper() : SSLCtx(role){};
};

base::LazyInstance<DefaultCtxWrapper<SSLRole::Client>> g_client_ctx;
base::LazyInstance<DefaultCtxWrapper<SSLRole::Client>> g_server_ctx;

}  // namespace

SSLCtx& SSLCtx::DefaultServerCtx() {
  return g_client_ctx.get();
}

SSLCtx& SSLCtx::DefaultClientCtx() {
  return g_client_ctx.get();
}

SSLCtx::Param::Param(SSLRole role) : role(role) {}

SSLCtx::SSLCtx(SSLRole role) : SSLCtx(Param(role)) {}

SSLCtx::SSLCtx(const Param& param) : param_(param) {
  std::call_once(lib_init_flag, open_ssl_lib_init);

  impl_ = SSL_CTX_new(TLS_method());
  SSL_CTX_set_options(impl_, SSL_OP_NO_SSLv2);
  SSL_CTX_set_options(impl_, SSL_OP_NO_SSLv3);
  SSL_CTX_set_session_cache_mode(impl_,
                                 SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_SERVER |
                                     SSL_SESS_CACHE_NO_INTERNAL |
                                     SSL_SESS_CACHE_NO_AUTO_CLEAR);

  int mode = SSL_VERIFY_NONE;
  if (param.ca_file.size() || param.ca_path.size()) {
    if (!SSL_CTX_load_verify_locations(impl_,
                                       param.ca_file.c_str(),
                                       param.ca_path.c_str())) {
      LOG(ERROR) << "ssl ca_file/ca_path failed!"
                 << ", file:" << param.ca_file << " path:" << param.ca_path;
      goto error;
    }
  }
  if (param.crt_file.size()) {
    if (!SSL_CTX_use_certificate_chain_file(impl_, param.crt_file.c_str())) {
      LOG(ERROR) << "ssl crt_file failed!" << param.crt_file;
      goto error;
    }
  }
  if (param.key_file.size()) {
    if (!SSL_CTX_use_PrivateKey_file(impl_,
                                     param.key_file.c_str(),
                                     SSL_FILETYPE_PEM)) {
      LOG(ERROR) << "ssl crt_file failed!" << param.crt_file;
      goto error;
    }
    if (!SSL_CTX_check_private_key(impl_)) {
      LOG(ERROR) << "ssl key_file check failed!";
      goto error;
    }
  }

  if (param.verify_peer) {
    mode = SSL_VERIFY_PEER;
  }
  if (mode == SSL_VERIFY_PEER && param.ca_file.empty() &&
      param.ca_path.empty()) {
    SSL_CTX_set_default_verify_paths(impl_);
  }
  SSL_CTX_set_verify(impl_, mode, NULL);

  // SSL_CTX_set1_sigalgs_list(ctx, "ECDSA+SHA256:RSA+SHA256");

  print_openssl_err();
  return;
error:
  print_openssl_err();
  impl_ = nullptr;
}

SSLCtx::~SSLCtx() {
  if (impl_) {
    SSL_CTX_free(impl_);
  }
  impl_ = nullptr;
}

SSLImpl* SSLCtx::NewSSLSession(int fd) {
  SSLImpl* ssl = SSL_new(impl_);
  SSL_set_fd(ssl, fd);
  return ssl;
}

void SSLCtx::SetCtxTimeout(int sec) {
  SSL_CTX_set_timeout(impl_, sec);
}

bool SSLCtx::UseCertification(const std::string& cert, const std::string& key) {
  if (cert.size() && !SSL_CTX_use_certificate_chain_file(impl_, cert.c_str())) {
    LOG(ERROR) << "ssl crt_file failed!" << cert;
    goto error;
  }

  if (key.size()) {
    if (!SSL_CTX_use_PrivateKey_file(impl_, key.c_str(), SSL_FILETYPE_PEM)) {
      LOG(ERROR) << "ssl crt_file failed!" << key;
      goto error;
    }
    if (!SSL_CTX_check_private_key(impl_)) {
      LOG(ERROR) << "ssl key_file check failed!";
      goto error;
    }
  }
  return true;
error:
  print_openssl_err();
  return false;
}

}  // namespace base
