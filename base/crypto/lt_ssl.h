#ifndef _LT_BASE_CRYPTO_SSL_H_
#define _LT_BASE_CRYPTO_SSL_H_

#include "base/ltio_config.h"

#ifdef LTIO_WITH_OPENSSL
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>

using SSLImpl = struct ssl_st;
using SSLCtxImpl = struct ssl_ctx_st;

// Recommended general purpose "Modern compatibility" cipher suites by
// mozilla.
//
// https://wiki.mozilla.org/Security/Server_Side_TLS
constexpr char DEFAULT_CIPHER_LIST[] =
    "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-"
    "CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-"
    "SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-"
    "AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256";

#else  // none LTIO_WITH_OPENSSL

static_assert(false, "current only openssl support");

#endif  // end LTIO_WITH_OPENSSL

namespace base {

enum class SSLRole {
  Client,
  Server,
};

enum class SSLState {
  LT_SSL_OPENED,
  LT_SSL_ACCEPTING,
  LT_SSL_CONNECTING,
};

class SSLCtx {
public:
  struct Param {
    Param(SSLRole role);
    SSLRole role;
    bool verify_peer = false;
    std::string crt_file;
    std::string key_file;
    std::string ca_file;
    std::string ca_path;
  };

public:
  static SSLCtx& DefaultServerCtx();
  static SSLCtx& DefaultClientCtx();

public:
  ~SSLCtx();
  SSLCtx(SSLRole role);
  SSLCtx(const Param& param);

  SSLImpl* NewSSLSession(int fd);

  void SetCtxTimeout(int sec);

  void SetVerifyMode(int mode);

  bool UseVerifyCA(const std::string& ca, const std::string& verify_path);

  bool UseCertification(const std::string& cert, const std::string& key);

  SSLCtxImpl* NativeHandle() {return impl_;}
private:
  Param param_;
  SSLCtxImpl* impl_ = nullptr;
};

}  // namespace base

#endif  //_LT_BASE_CRYPTO_SSL_H_
