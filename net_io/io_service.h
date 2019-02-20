#ifndef _NET_IO_SERVICE_H_H
#define _NET_IO_SERVICE_H_H

#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include "net_callback.h"
#include "service_acceptor.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service.h"
#include "dispatcher/workload_dispatcher.h"
#include "base/message_loop/message_loop.h"

namespace net {

class IOService;
/* IOserviceDelegate Provide the some notify to its owner and got the IOWork loop for IO*/
class IOServiceDelegate {
public:
  virtual ~IOServiceDelegate(){};
  virtual base::MessageLoop* GetNextIOWorkLoop() = 0;

  /* use for couting connection numbers and limit
   * max connections; return false when reach limits,
   * otherwise return true */
  virtual bool IncreaseChannelCount() = 0;
  virtual void DecreaseChannelCount() = 0;

  virtual bool CanCreateNewChannel() { return true;}
  /* do other config things for this ioservice; return false will kill this service*/
  virtual bool BeforeIOServiceStart(IOService* ioservice) {return true;};
  virtual void IOServiceStarted(const IOService* ioservice) {};
  virtual void IOServiceStoped(const IOService* ioservice) {};
};

/* Every IOService own a acceptor and listing on a adress,
 * handle incomming connection from acceptor and manager
 * them on working-messageloop */
class IOService : public ProtoServiceDelegate {
public:
  /* Must Construct in ownerloop, why? bz we want all io level is clear and tiny
   * it only handle io relative things, it's easy! just post a task IOMain at everything
   * begin,
   *
   * A IOService Accept listen a local address and accept incoming connection; every connections
   * will bind to protocol service*/
  IOService(const SocketAddress local,
            const std::string protocol,
            base::MessageLoop* workloop,
            IOServiceDelegate* delegate);

  ~IOService();

  void StartIOService();
  void StopIOService();

  base::MessageLoop* AcceptorLoop() { return accept_loop_; }
  const std::string& IOServiceName() const {return service_name_;}
  bool IsRunning() {return acceptor_ && acceptor_->IsListening();}

  void SetProtoMessageHandler(ProtoMessageHandler handler);
private:
  //void HandleProtoMessage(RefProtocolMessage message);
  /* create a new connection channel */
  void OnNewConnection(int, const SocketAddress&);

  // override from ProtoServiceDelegate to manager[remove] from managed list
  void OnProtocolServiceGone(const RefProtoService& service) override;

  void StoreProtocolService(const RefProtoService);
  void RemoveProtocolService(const RefProtoService);

  //bool as_dispatcher_;
  std::string protocol_;

  SocketAcceptorPtr acceptor_;
  base::MessageLoop* accept_loop_;

  /* interface to owner and handler */
  IOServiceDelegate* delegate_;
  //RefProtoService proto_service_;
  bool is_stopping_ = false;

  // install this callback to protoservice
  ProtoMessageHandler message_handler_;

  uint64_t channel_count_;
  std::string service_name_;
  std::unordered_set<RefProtoService> protocol_services;
};

}
#endif
