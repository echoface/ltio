#ifndef LT_NET_HTTP2_TLS_CONTEXT
#define LT_NET_HTTP2_TLS_CONTEXT

#include "base/ltio_config.h"
#include "base/crypto/lt_ssl.h"

namespace lt {
namespace net {

bool configure_clien_tls_context(SSLCtxImpl* ssl_ctx);

bool configure_server_tls_context_easy(SSLCtxImpl* ctx);

}  // namespace net
}  // namespace lt

#endif
