#ifndef HV_WS_DEF_H_
#define HV_WS_DEF_H_

#include <stdbool.h>
#include <stdlib.h>  // import rand
#include <string>

#define SEC_WEBSOCKET_VERSION "Sec-WebSocket-Version"
#define SEC_WEBSOCKET_KEY "Sec-WebSocket-Key"
#define SEC_WEBSOCKET_ACCEPT "Sec-WebSocket-Accept"

#define WS_SERVER_MIN_FRAME_SIZE 2
// 1000 1001 0000 0000
#define WS_SERVER_PING_FRAME "\211\0"
// 1000 1010 0000 0000
#define WS_SERVER_PONG_FRAME "\212\0"

#define WS_CLIENT_MIN_FRAME_SIZE 6
// 1000 1001 1000 0000
#define WS_CLIENT_PING_FRAME "\211\200WSWS"
// 1000 1010 1000 0000
#define WS_CLIENT_PONG_FRAME "\212\200WSWS"

enum ws_session_type {
  WS_CLIENT,
  WS_SERVER,
};

enum ws_opcode {
  WS_OPCODE_CONTINUE = 0x0,
  WS_OPCODE_TEXT = 0x1,
  WS_OPCODE_BINARY = 0x2,
  WS_OPCODE_CLOSE = 0x8,
  WS_OPCODE_PING = 0x9,
  WS_OPCODE_PONG = 0xA,
};

struct WebsocketUtil {
  // Sec-WebSocket-Key => Sec-WebSocket-Accept
  static std::string GenAcceptKey(const char* key);

  // fix-header[2] + var-length[2/8] + mask[4] + data[data_len]
  static int CalculateFrameSize(int data_len, bool has_mask = false);

  static int BuildFrame(char* out,
                        const char* data,
                        int data_len,
                        const char mask[4],
                        bool has_mask = false,
                        enum ws_opcode opcode = WS_OPCODE_TEXT,
                        bool fin = true);

  static int BuildClientFrame(char* out,
                              const char* data,
                              int data_len,
                              /* const char mask[4] */
                              /* bool has_mask = true */
                              enum ws_opcode opcode = (WS_OPCODE_TEXT),
                              bool fin = (true)) {
    char mask[4];
    *(int*)mask = rand();
    return BuildFrame(out, data, data_len, mask, true, opcode, fin);
  }

  static int BuildServerFrame(char* out,
                              const char* data,
                              int data_len,
                              /* const char mask[4] */
                              /* bool has_mask = false */
                              enum ws_opcode opcode = (WS_OPCODE_TEXT),
                              bool fin = (true)) {
    char mask[4] = {0};
    return BuildFrame(out, data, data_len, mask, false, opcode, fin);
  }
};

#endif  // HV_WS_DEF_H_
