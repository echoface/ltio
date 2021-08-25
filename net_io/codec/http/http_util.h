
#include <string>

#include "llhttp/include/llhttp.h"

namespace lt {
namespace net {

class HttpUitl {
  static bool expect_response_body(int status_code) {
    return status_code == 101 ||
           (status_code / 100 != 1 && status_code != 304 && status_code != 204);
  }

  static bool expect_response_body(const std::string& method, int status_code) {
    return method != "HEAD" && expect_response_body(status_code);
  }

  static bool expect_response_body(int method_token, int status_code) {
    return method_token != HTTP_HEAD && expect_response_body(status_code);
  }
};

}  // namespace net
}  // namespace lt
