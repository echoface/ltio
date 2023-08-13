#ifndef LT_NET_HTTP2_TLS_CONTEXT
#define LT_NET_HTTP2_TLS_CONTEXT

/*
this ssl configure by referenece: nghttp2/src/HttpServer.cc
*/

#include "h2_tls_context.h"

#include <cstddef>
#include <vector>

#include <nghttp2/nghttp2.h>
#include <openssl/ssl.h>

#include "base/crypto/lt_ssl.h"
#include "base/string/string_view.h"

using string_view = std::string_view;

namespace lt {
namespace net {

namespace {
template <typename T, size_t N>
constexpr size_t str_size(T (&)[N]) {
  return N - 1;
}

template <typename CharT, size_t N>
constexpr static string_view sv_from_lit(const CharT (&s)[N]) {
  return string_view{s, N - 1};
}

constexpr auto NGHTTP2_H2_ALPN = sv_from_lit("\x2h2");
constexpr auto NGHTTP2_H2 = sv_from_lit("h2");
// The additional HTTP/2 protocol ALPN protocol identifier we also
// supports for our applications to make smooth migration into final
// h2 ALPN ID.
constexpr auto NGHTTP2_H2_16_ALPN = sv_from_lit("\x5h2-16");
constexpr auto NGHTTP2_H2_16 = sv_from_lit("h2-16");

constexpr auto NGHTTP2_H2_14_ALPN = sv_from_lit("\x5h2-14");
constexpr auto NGHTTP2_H2_14 = sv_from_lit("h2-14");

constexpr auto NGHTTP2_H1_1_ALPN = sv_from_lit("\x8http/1.1");
constexpr auto NGHTTP2_H1_1 = sv_from_lit("http/1.1");

constexpr size_t NGHTTP2_MAX_UINT64_DIGITS = str_size("18446744073709551615");

bool select_proto(const unsigned char** out,
                  unsigned char* outlen,
                  const unsigned char* in,
                  unsigned int inlen,
                  const string_view& key) {
  for (auto p = in, end = in + inlen; p + key.size() <= end; p += *p + 1) {
    if (std::equal(std::begin(key), std::end(key), p)) {
      *out = p + 1;
      *outlen = *p;
      return true;
    }
  }
  return false;
}

bool select_h2(const unsigned char** out,
               unsigned char* outlen,
               const unsigned char* in,
               unsigned int inlen) {
  return select_proto(out, outlen, in, inlen, NGHTTP2_H2_ALPN) ||
         select_proto(out, outlen, in, inlen, NGHTTP2_H2_16_ALPN) ||
         select_proto(out, outlen, in, inlen, NGHTTP2_H2_14_ALPN);
}

bool select_protocol(const unsigned char** out,
                     unsigned char* outlen,
                     const unsigned char* in,
                     unsigned int inlen,
                     std::vector<std::string> proto_list) {
  for (const auto& proto : proto_list) {
    if (select_proto(out, outlen, in, inlen, string_view{proto})) {
      return true;
    }
  }
  return false;
}

#ifndef OPENSSL_NO_NEXTPROTONEG
std::vector<unsigned char> get_default_alpn() {
  auto res = std::vector<unsigned char>(NGHTTP2_H2_ALPN.size() +
                                        NGHTTP2_H2_16_ALPN.size() +
                                        NGHTTP2_H2_14_ALPN.size());
  auto p = std::begin(res);

  p = std::copy_n(std::begin(NGHTTP2_H2_ALPN), NGHTTP2_H2_ALPN.size(), p);
  p = std::copy_n(std::begin(NGHTTP2_H2_16_ALPN), NGHTTP2_H2_16_ALPN.size(), p);
  p = std::copy_n(std::begin(NGHTTP2_H2_14_ALPN), NGHTTP2_H2_14_ALPN.size(), p);

  return res;
}

std::vector<unsigned char>& get_alpn_token() {
  static auto alpn_token = get_default_alpn();
  return alpn_token;
}

/*
 * Callback function for TLS NPN. Since this program only supports
 * HTTP/2 protocol, if server does not offer HTTP/2 the nghttp2
 * library supports, we terminate program.
 */
static int select_next_proto_cb(SSL* ssl,
                                unsigned char** out,
                                unsigned char* outlen,
                                const unsigned char* in,
                                unsigned int inlen,
                                void* arg) {
  (void)ssl;
  (void)arg;
  /* nghttp2_select_next_protocol() selects HTTP/2 protocol the
     nghttp2 library supports. */
  if (nghttp2_select_next_protocol(out, outlen, in, inlen) <= 0) {
    return SSL_TLSEXT_ERR_ALERT_FATAL;
  }
  return SSL_TLSEXT_ERR_OK;
}

int client_select_next_proto_cb(SSL* ssl,
                                unsigned char** out,
                                unsigned char* outlen,
                                const unsigned char* in,
                                unsigned int inlen,
                                void* arg) {
  if (!select_h2(const_cast<const unsigned char**>(out), outlen, in, inlen)) {
    return SSL_TLSEXT_ERR_NOACK;
  }
  return SSL_TLSEXT_ERR_OK;
}
#endif /* !OPENSSL_NO_NEXTPROTONEG */

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
int alpn_select_proto_cb(SSL* ssl,
                         const unsigned char** out,
                         unsigned char* outlen,
                         const unsigned char* in,
                         unsigned int inlen,
                         void* arg) {
  if (!select_h2(out, outlen, in, inlen)) {
    return SSL_TLSEXT_ERR_NOACK;
  }
  return SSL_TLSEXT_ERR_OK;
}
#endif  // OPENSSL_VERSION_NUMBER >= 0x10002000L

}  // namespace

// for server SSLCtx easy configure
bool configure_server_tls_context_easy(SSLCtxImpl* ctx) {
  auto ssl_opts = (SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS) |
                  SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TICKET |
                  SSL_OP_NO_COMPRESSION | SSL_OP_SINGLE_ECDH_USE |
                  SSL_OP_CIPHER_SERVER_PREFERENCE |
                  SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
  SSL_CTX_set_options(ctx, ssl_opts);
  SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
  SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS);
  SSL_CTX_set_cipher_list(ctx, DEFAULT_CIPHER_LIST);

#ifndef OPENSSL_NO_NEXTPROTONEG
  SSL_CTX_npn_advertised_cb_func npn_fun =
      [](SSL* s, const unsigned char** data, unsigned int* len, void* arg) {
        auto& token = get_alpn_token();
        *data = token.data();
        *len = token.size();
        return SSL_TLSEXT_ERR_OK;
      };
  SSL_CTX_set_next_protos_advertised_cb(ctx, npn_fun, nullptr);
#endif  // !OPENSSL_NO_NEXTPROTONEG

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
  SSL_CTX_set_alpn_select_cb(ctx, alpn_select_proto_cb, nullptr);
#endif  // OPENSSL_VERSION_NUMBER >= 0x10002000L
  return true;
}

// for client SSLCtx, provider a supported protocol list
bool configure_clien_tls_context(SSLCtxImpl* ssl_ctx) {
#ifndef OPENSSL_NO_NEXTPROTONEG
  SSL_CTX_set_next_proto_select_cb(ssl_ctx,
                                   client_select_next_proto_cb,
                                   nullptr);
#endif  // !OPENSSL_NO_NEXTPROTONEG

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
  auto proto_list = get_default_alpn();
  SSL_CTX_set_alpn_protos(ssl_ctx, proto_list.data(), proto_list.size());
#endif  // OPENSSL_VERSION_NUMBER >= 0x10002000L
  return true;
}

}  // namespace net
}  // namespace lt

#endif
