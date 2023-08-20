/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NET_CALLBACK_DEF_H_H
#define _NET_CALLBACK_DEF_H_H

#include <functional>
#include <memory>
#include <string>

#include "common/ip_endpoint.h"

namespace lt {
namespace net {

class IOBuffer;
class IPEndPoint;
class SocketAcceptor;
class TcpChannel;
class IOService;
class CodecService;
class CodecMessage;

/* ============= Service Acceptor relative ===========*/
typedef std::shared_ptr<SocketAcceptor> RefSocketAcceptor;
typedef std::unique_ptr<SocketAcceptor> SocketAcceptorPtr;
typedef std::function<void(int /*socket_fd*/, const IPEndPoint&)>
    NewConnectionCallback;

/* protoservice handle tcpmessage to different type protocol request and message
 */
typedef std::unique_ptr<CodecService> CodecServicePtr;
typedef std::shared_ptr<CodecService> RefCodecService;
typedef std::weak_ptr<CodecService> WeakCodecService;

/* ============== io service ====== */
typedef std::shared_ptr<IOService> RefIOService;

}  // namespace net
}  // namespace lt
#endif
