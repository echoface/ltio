
#include "initializer.h"

namespace lt {
namespace net {

Initializer::~Initializer() {}
Initializer::Initializer(const url::RemoteInfo& remote, const ClientConfig& config)
  : remote_(remote),
    config_(config) {
}

void Initializer::Init(RefProtoService& service) {
  service->SetDelegate(this);
  client_serivces_.insert(service);
}

void Initializer::OnProtocolServiceReady(const RefProtoService& service) {
  if (success_fn_) {
    RefProtoService guard(service);
    client_serivces_.erase(service);
    success_fn_(guard);
  }
}

void Initializer::OnProtocolServiceGone(const RefProtoService& service) {
  client_serivces_.erase(service);
}

void Initializer::OnProtocolMessage(const RefProtocolMessage& message) {

}


}} // lt:net
