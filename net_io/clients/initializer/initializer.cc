
#include "initializer.h"

namespace lt {
namespace net {

Initializer::~Initializer() {}

void Initializer::Init(RefProtoService& service) {
  service->SetDelegate(this);
  client_serivces_.insert(service);
}

void Initializer::OnProtocolServiceReady(const RefProtoService& service) {
  client_serivces_.erase(service);
  provider_->OnClientServiceReady(service);
}

void Initializer::OnProtocolServiceGone(const RefProtoService& service) {
  client_serivces_.erase(service);
}

void Initializer::OnProtocolMessage(const RefProtocolMessage& message) {
  RefProtoService service = message->GetIOCtx().protocol_service.lock();
  if (!service) {
    return;
  }

  client_serivces_.erase(service);
  provider_->OnClientServiceReady(service);
}


}} // lt:net
