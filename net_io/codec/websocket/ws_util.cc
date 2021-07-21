#include "ws_util.h"

#include <hash/sha1.h>
#include <string.h>
#include <base/utils/base64/base64.hpp>

#include "websocket_parser.h"

namespace {
  const char *ws_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
}

// base64_encode( SHA1(key + magic) )
std::string WebsocketUtil::GenAcceptKey(const char* key) {
  base::SecureHashAlgorithm sha1;
  sha1.Update(key, strlen(key));
  sha1.Update(ws_magic, strlen(ws_magic));
  sha1.Final();
  return base::base64_encode(sha1.Digest(), 20);
}

// fix-header[2] + var-length[2/8] + mask[4] + data[data_len]
int WebsocketUtil::CalculateFrameSize(int data_len, bool has_mask) {
  int size = data_len + 2;
  if (data_len >= 126) {
    size += (data_len > 0xFFFF ? 8 : 2);
  }
  return has_mask ? size + 4 : size;
}

int WebsocketUtil::BuildFrame(char* out,
                              const char* data,
                              int data_len,
                              const char mask[4],
                              bool has_mask,
                              enum ws_opcode opcode,
                              bool fin) {
  int flags = opcode;
  if (fin)
    flags |= WS_FIN;
  if (has_mask)
    flags |= WS_HAS_MASK;
  return websocket_build_frame(out,
                               (websocket_flags)flags,
                               mask,
                               data,
                               data_len);
}
