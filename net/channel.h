//
// Created by gh on 18-12-5.
//

#ifndef LIGHTINGIO_NET_CHANNEL_H
#define LIGHTINGIO_NET_CHANNEL_H

// socket chennel interface and base class
namespace net {

class ChannelConsumer {
public:
	virtual void OnStatusChanged(const RefTcpChannel&) {};
	virtual void OnDataFinishSend(const RefTcpChannel&) {};
	virtual void OnChannelClosed(const RefTcpChannel&) = 0;
	virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*) = 0;
};

}

#endif //LIGHTINGIO_NET_CHANNEL_H
