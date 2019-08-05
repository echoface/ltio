//
// Created by gh on 18-12-5.
//

#ifndef LIGHTINGIO_NET_CHANNEL_H
#define LIGHTINGIO_NET_CHANNEL_H

#include "net_callback.h"

// socket chennel interface and base class
namespace lt {
namespace net {

class ChannelConsumer {
public:
	virtual void OnStatusChanged(const RefTcpChannel&) {};
	virtual void OnDataFinishSend(const RefTcpChannel&) {};
	virtual void OnChannelClosed(const RefTcpChannel&) = 0;
	virtual void OnDataReceived(const RefTcpChannel &, IOBuffer *) = 0;
};

}}
#endif //LIGHTINGIO_NET_CHANNEL_H
