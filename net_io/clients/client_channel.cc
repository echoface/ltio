//
// Created by gh on 18-12-15.
//

#include "client_channel.h"
#include "async_channel.h"
#include "queued_channel.h"

namespace net {

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate, RefProtoService& service) {
	if (service->KeepSequence()) {
		auto client_channel = QueuedChannel::Create(delegate, service);
		return std::static_pointer_cast<ClientChannel>(client_channel);
	}
	auto client_channel = AsyncChannel::Create(delegate, service);
	return std::static_pointer_cast<ClientChannel>(client_channel);
}


ClientChannel::ClientChannel(Delegate* d, RefProtoService& service)
		: delegate_(d),
		  protocol_service_(service) {
}

void ClientChannel::CloseClientChannel() {
	base::MessageLoop* io = IOLoop();
	if (io->IsInLoopThread()) {
		protocol_service_->CloseService();
		return;
	}
	io->PostTask(NewClosure(std::bind(&ProtoService::CloseService, protocol_service_)));
}

}
