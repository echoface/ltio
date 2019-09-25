#include "redis_initializer.h"
#include "protocol/redis/resp_service.h"
#include "protocol/redis/redis_request.h"

namespace lt {
namespace net {

RedisInitializer::~RedisInitializer() {}

void RedisInitializer::OnProtocolServiceReady(const RefProtoService& service) {
  const url::RemoteInfo& remote = provider_->GetRemoteInfo();
  //const ClientConfig& config = provider_->GetClientConfig();
  const auto& iter = remote.querys.find("db");
  if (iter == remote.querys.end()) {
    client_serivces_.erase(service);
    return provider_->OnClientServiceReady(service);
  }
  RefRedisRequest request = std::make_shared<RedisRequest>();
  request->Select(iter->second);
  if (false == service->SendRequestMessage(RefCast(ProtocolMessage, request))) {
    service->CloseService();
  }
}

void RedisInitializer::OnProtocolServiceGone(const RefProtoService& service) {
  auto iter = client_serivces_.find(service);
  if (iter == client_serivces_.end()) {
    return;
  }
  client_serivces_.erase(iter);
}

void RedisInitializer::OnProtocolMessage(const RefProtocolMessage& message) {
  RedisResponse* response = (RedisResponse*)message.get();
  auto service = response->GetIOCtx().protocol_service.lock();
  if (response->Count() != 1) {
    service->CloseService();
    return;
  }

  auto& value = response->ResultAtIndex(0);
  if (value.type() == resp::ty_string &&
      value.string().data() == std::string("OK")) {
    client_serivces_.erase(service);
    provider_->OnClientServiceReady(service);
  } else {
    service->CloseService();
  }
}


}}//lt::net
