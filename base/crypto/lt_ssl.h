#ifndef _LT_BASE_CRYPTO_SSL_H_
#define _LT_BASE_CRYPTO_SSL_H_

#ifdef LTIO_WITH_OPENSSL
#include <string>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>

using SSLImpl = struct ssl_st;
using SSLCtxImpl = struct ssl_ctx_st;

#else   // none LTIO_WITH_OPENSSL

astatic_assert(false, "current only openssl support")

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

  bool UseCertification(const std::string& cert, const std::string& key);
private:
  Param param_;
  SSLCtxImpl* impl_ = nullptr;
};

}  // namespace base

#endif  //_LT_BASE_CRYPTO_SSL_H_
