
#include "proto_service.h"
#include "proto_message.h"
#include "glog/logging.h"
#include "net/tcp_channel.h"

namespace net {
ProtoService::ProtoService()
	: type_(ProtocolServiceType::kServer) {
}
ProtoService::~ProtoService() {};

void ProtoService::SetServiceType(ProtocolServiceType t) {
  type_ = t;
}
void ProtoService::SetChannelWriter(ChannelWriter* writer) {
	writer_ = writer;
}

void ProtoService::SetMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

void ProtoService::BeforeWriteMessage(ProtocolMessage* message) {
	return IsServerSideservice() ? BeforeWriteResponseToBuffer(message) : BeforeWriteRequestToBuffer(message);
}

}// end namespace
