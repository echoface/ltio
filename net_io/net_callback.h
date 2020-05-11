#ifndef _NET_CALLBACK_DEF_H_H
#define _NET_CALLBACK_DEF_H_H

#include "base/ip_endpoint.h"
#include <string>
#include <memory>
#include <functional>

namespace lt {
namespace net {

class IOBuffer;
class IPEndPoint;
class SocketAcceptor;
class TcpChannel;
class IOService;
class CodecService;
class CodecMessage;


/* ============ connection channel relative =========*/
typedef std::unique_ptr<TcpChannel> TcpChannelPtr;


/* ============= Service Acceptor relative ===========*/
typedef std::shared_ptr<SocketAcceptor> RefSocketAcceptor;
typedef std::unique_ptr<SocketAcceptor> SocketAcceptorPtr;
typedef std::function<void(int/*socket_fd*/, const IPEndPoint&)> NewConnectionCallback;

/* protoservice handle tcpmessage to different type protocol request and message */
typedef std::unique_ptr<CodecService> CodecServicePtr;
typedef std::shared_ptr<CodecService> RefCodecService;
typedef std::weak_ptr<CodecService> WeakCodecService;

/* ============== io service ====== */
typedef std::shared_ptr<IOService> RefIOService;

}}
#endif
