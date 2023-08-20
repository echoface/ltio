/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NET_IO_SERVICE_H_H
#define _NET_IO_SERVICE_H_H

#include <atomic>
#include <unordered_set>

#include "common/ip_endpoint.h"
#include "codec/codec_message.h"
#include "codec/codec_service.h"
#include "net_callback.h"
#include "socket_acceptor.h"

#ifdef LTIO_HAVE_SSL
#include "base/crypto/lt_ssl.h"
#endif

namespace lt {
namespace net {

class IOService;

/* IOserviceDelegate Provide the some notify to its owner and got the IOWork
 * loop for IO*/
class IOServiceDelegate {
public:
  virtual ~IOServiceDelegate(){};
  virtual base::MessageLoop* GetNextIOWorkLoop() = 0;

  /* use for couting connection numbers and limit
   * max connections; return false when reach limits,
   * otherwise return true */
  virtual bool CanCreateNewChannel() { return true; }

  virtual void IncreaseChannelCount() = 0;
  virtual void DecreaseChannelCount() = 0;
  /* do other config things for this ioservice; return false will kill this
   * service*/
  virtual void IOServiceStarted(const IOService* ioservice){};
  virtual void IOServiceStoped(const IOService* ioservice){};

#ifdef LTIO_HAVE_SSL  // server should provider this
  virtual base::SSLCtx* GetSSLContext() { return nullptr; }
#endif

  // codec/channel is ready, can read/write
  virtual void OnConnectionOpen(const RefCodecService& service) {};

  // codec/channel closed, any read/write operation not allowed
  virtual void OnConnectionClose(const RefCodecService& service) {};
};

/* Every IOService own a acceptor and listing on a adress,
 * handle incomming connection from acceptor and manager
 * them on working-messageloop */
class IOService : public EnableShared(IOService),
                  public SocketAcceptor::Actor,
                  public CodecService::Delegate {
public:
  using Handler = CodecService::Handler;
  /* Must Construct in ownerloop, why? bz we want all io level is clear and tiny
   * it only handle io relative things, it's easy! just post a task IOMain at
   * everything begin,
   *
   * A IOService Accept listen a local address and accept incoming connection;
   * every connections will bind to protocol service*/
  IOService(const IPEndPoint& local,
            const std::string protocol,
            base::MessageLoop* workloop,
            IOServiceDelegate* delegate);

  ~IOService();

  IOService& WithHandler(Handler* h);

  void Start();

  void Stop();

  base::MessageLoop* AcceptorLoop() { return acpt_io_; }

  const std::string IOServiceName() const { return endpoint_.ToString(); }

  bool IsRunning() { return acceptor_ && acceptor_->IsListening(); }

private:
  // void HandleProtoMessage(RefCodecMessage message);
  /* create a new connection channel */
  void OnNewConnection(int, const IPEndPoint&) override;

  // override from CodecService::Delegate to manager[remove] from managed list
  void OnCodecClosed(const RefCodecService& service) override;

  // override from CodecService::Delegate to give a chance for server push
  // eg: bi-stream handleshake
  void OnCodecReady(const RefCodecService& service) override;

  void StoreProtocolService(const RefCodecService);

  void RemoveProtocolService(const RefCodecService);

  RefCodecService CreateCodeService(int fd, const IPEndPoint& ep);

  // bool as_dispatcher_;
  std::string protocol_;

  SocketAcceptorPtr acceptor_;
  base::MessageLoop* acpt_io_;

  CodecService::Handler* handler_ = nullptr;

  /* interface to owner and handler */
  IOServiceDelegate* delegate_;

  bool is_stopping_ = false;

  uint64_t channel_count_;

  IPEndPoint endpoint_;

  std::unordered_set<RefCodecService> codecs_;
};

}  // namespace net
}  // namespace lt
#endif
