#include "nghttp2_util.h"
#include "fmt/core.h"

namespace util {
template <typename InputIt1, typename InputIt2>
bool streq(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (std::distance(first1, last1) != std::distance(first2, last2)) {
    return false;
  }
  return std::equal(first1, last1, first2);
}

template <typename T, typename S>
bool streq(const T& a, const S& b) {
  return streq(a.begin(), a.end(), b.begin(), b.end());
}

template <typename CharT, typename InputIt, size_t N>
bool streq_l(const CharT (&a)[N], InputIt b, size_t blen) {
  return streq(a, a + (N - 1), b, b + blen);
}

template <typename CharT, size_t N, typename T>
bool streq_l(const CharT (&a)[N], const T& b) {
  return streq(a, a + (N - 1), b.begin(), b.end());
}

}  // namespace util

namespace lt {
namespace net {

int lookup_token(const uint8_t* name, size_t namelen) {
  switch (namelen) {
    case 2:
      switch (name[1]) {
        case 'e':
          if (util::streq_l("t", name, 1)) {
            return HD_TE;
          }
          break;
      }
      break;
    case 3:
      switch (name[2]) {
        case 'a':
          if (util::streq_l("vi", name, 2)) {
            return HD_VIA;
          }
          break;
      }
      break;
    case 4:
      switch (name[3]) {
        case 'e':
          if (util::streq_l("dat", name, 3)) {
            return HD_DATE;
          }
          break;
        case 'k':
          if (util::streq_l("lin", name, 3)) {
            return HD_LINK;
          }
          break;
        case 't':
          if (util::streq_l("hos", name, 3)) {
            return HD_HOST;
          }
          break;
      }
      break;
    case 5:
      switch (name[4]) {
        case 'h':
          if (util::streq_l(":pat", name, 4)) {
            return HD__PATH;
          }
          break;
        case 't':
          if (util::streq_l(":hos", name, 4)) {
            return HD__HOST;
          }
          break;
      }
      break;
    case 6:
      switch (name[5]) {
        case 'e':
          if (util::streq_l("cooki", name, 5)) {
            return HD_COOKIE;
          }
          break;
        case 'r':
          if (util::streq_l("serve", name, 5)) {
            return HD_SERVER;
          }
          break;
        case 't':
          if (util::streq_l("expec", name, 5)) {
            return HD_EXPECT;
          }
          break;
      }
      break;
    case 7:
      switch (name[6]) {
        case 'c':
          if (util::streq_l("alt-sv", name, 6)) {
            return HD_ALT_SVC;
          }
          break;
        case 'd':
          if (util::streq_l(":metho", name, 6)) {
            return HD__METHOD;
          }
          break;
        case 'e':
          if (util::streq_l(":schem", name, 6)) {
            return HD__SCHEME;
          }
          if (util::streq_l("upgrad", name, 6)) {
            return HD_UPGRADE;
          }
          break;
        case 'r':
          if (util::streq_l("traile", name, 6)) {
            return HD_TRAILER;
          }
          break;
        case 's':
          if (util::streq_l(":statu", name, 6)) {
            return HD__STATUS;
          }
          break;
      }
      break;
    case 8:
      switch (name[7]) {
        case 'n':
          if (util::streq_l("locatio", name, 7)) {
            return HD_LOCATION;
          }
          break;
      }
      break;
    case 9:
      switch (name[8]) {
        case 'd':
          if (util::streq_l("forwarde", name, 8)) {
            return HD_FORWARDED;
          }
          break;
        case 'l':
          if (util::streq_l(":protoco", name, 8)) {
            return HD__PROTOCOL;
          }
          break;
      }
      break;
    case 10:
      switch (name[9]) {
        case 'a':
          if (util::streq_l("early-dat", name, 9)) {
            return HD_EARLY_DATA;
          }
          break;
        case 'e':
          if (util::streq_l("keep-aliv", name, 9)) {
            return HD_KEEP_ALIVE;
          }
          break;
        case 'n':
          if (util::streq_l("connectio", name, 9)) {
            return HD_CONNECTION;
          }
          break;
        case 't':
          if (util::streq_l("user-agen", name, 9)) {
            return HD_USER_AGENT;
          }
          break;
        case 'y':
          if (util::streq_l(":authorit", name, 9)) {
            return HD__AUTHORITY;
          }
          break;
      }
      break;
    case 12:
      switch (name[11]) {
        case 'e':
          if (util::streq_l("content-typ", name, 11)) {
            return HD_CONTENT_TYPE;
          }
          break;
      }
      break;
    case 13:
      switch (name[12]) {
        case 'l':
          if (util::streq_l("cache-contro", name, 12)) {
            return HD_CACHE_CONTROL;
          }
          break;
      }
      break;
    case 14:
      switch (name[13]) {
        case 'h':
          if (util::streq_l("content-lengt", name, 13)) {
            return HD_CONTENT_LENGTH;
          }
          break;
        case 's':
          if (util::streq_l("http2-setting", name, 13)) {
            return HD_HTTP2_SETTINGS;
          }
          break;
      }
      break;
    case 15:
      switch (name[14]) {
        case 'e':
          if (util::streq_l("accept-languag", name, 14)) {
            return HD_ACCEPT_LANGUAGE;
          }
          break;
        case 'g':
          if (util::streq_l("accept-encodin", name, 14)) {
            return HD_ACCEPT_ENCODING;
          }
          break;
        case 'r':
          if (util::streq_l("x-forwarded-fo", name, 14)) {
            return HD_X_FORWARDED_FOR;
          }
          break;
      }
      break;
    case 16:
      switch (name[15]) {
        case 'n':
          if (util::streq_l("proxy-connectio", name, 15)) {
            return HD_PROXY_CONNECTION;
          }
          break;
      }
      break;
    case 17:
      switch (name[16]) {
        case 'e':
          if (util::streq_l("if-modified-sinc", name, 16)) {
            return HD_IF_MODIFIED_SINCE;
          }
          break;
        case 'g':
          if (util::streq_l("transfer-encodin", name, 16)) {
            return HD_TRANSFER_ENCODING;
          }
          break;
        case 'o':
          if (util::streq_l("x-forwarded-prot", name, 16)) {
            return HD_X_FORWARDED_PROTO;
          }
          break;
        case 'y':
          if (util::streq_l("sec-websocket-ke", name, 16)) {
            return HD_SEC_WEBSOCKET_KEY;
          }
          break;
      }
      break;
    case 20:
      switch (name[19]) {
        case 't':
          if (util::streq_l("sec-websocket-accep", name, 19)) {
            return HD_SEC_WEBSOCKET_ACCEPT;
          }
          break;
      }
      break;
  }
  return -1;
}

void H2Util::PrintFrameHeader(const nghttp2_frame_hd* hd) {
  LOG(INFO) << fmt::format(
      "[frame] length={:d} type={:x} flags={:x} stream_id={:d}",
      (int)hd->length,
      (int)hd->type,
      (int)hd->flags,
      hd->stream_id);
}

}  // namespace net
}  // namespace lt
