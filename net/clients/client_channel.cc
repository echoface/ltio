//
// Created by gh on 18-12-15.
//

#include "client_channel.h"

namespace net {

void ClientChannel::CloseClientChannel() {
	base::MessageLoop* io = IOLoop();
	if (io->IsInLoopThread()) {
		protocol_service_->CloseService();
		return;
	}
	io->PostTask(NewClosure(std::bind(&ProtoService::CloseService, protocol_service_)));
}

}
