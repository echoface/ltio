#ifndef _NET_CALLBACK_DEF_H_H
#define _NET_CALLBACK_DEF_H_H

#include <string>
#include <memory>
#include <functional>

namespace lt {
namespace net {

class IOBuffer;
class SocketAddr;
class SocketAcceptor;
class TcpChannel;
class IOService;
class CodecService;
class CodecMessage;


/* ============ connection channel relative =========*/
typedef std::shared_ptr<TcpChannel> RefTcpChannel;
typedef std::weak_ptr<TcpChannel> WeakPtrTcpChannel;

typedef std::function<void(const RefTcpChannel&)> FinishSendCallback;
typedef std::function<void(const RefTcpChannel&)> ChannelClosedCallback;
typedef std::function<void(const RefTcpChannel&)> ChannelStatusCallback;
typedef std::function<void(RefTcpChannel&, IOBuffer*)> RcvDataCallback;


/* ============= Service Acceptor relative ===========*/
typedef std::shared_ptr<SocketAcceptor> RefSocketAcceptor;
typedef std::unique_ptr<SocketAcceptor> SocketAcceptorPtr;
typedef std::function<void(int/*socket_fd*/, const SocketAddr&)> NewConnectionCallback;

/* protoservice handle tcpmessage to different type protocol request and message */
typedef std::unique_ptr<CodecService> CodecServicePtr;
typedef std::shared_ptr<CodecService> RefCodecService;
typedef std::weak_ptr<CodecService> WeakCodecService;

/* ============== io service ====== */
typedef std::shared_ptr<IOService> RefIOService;
typedef std::unique_ptr<IOService> IOServicePtr;

}}
#endif
