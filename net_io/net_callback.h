#ifndef _NET_CALLBACK_DEF_H_H
#define _NET_CALLBACK_DEF_H_H

#include <string>
#include <memory>
#include <functional>

namespace lt {
namespace net {

class IOBuffer;
class SocketAddress;
class SocketAcceptor;
class TcpChannel;
class IOService;
class ProtoService;
class ProtocolMessage;


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
typedef std::function<void(int/*socket_fd*/, const SocketAddress&)> NewConnectionCallback;

/* protoservice handle tcpmessage to different type protocol request and message */
typedef std::unique_ptr<ProtoService> ProtoServicePtr;
typedef std::shared_ptr<ProtoService> RefProtoService;
typedef std::weak_ptr<ProtoService> WeakProtoService;

/* ============== io service ====== */
typedef std::shared_ptr<IOService> RefIOService;
typedef std::unique_ptr<IOService> IOServicePtr;

}}
#endif
