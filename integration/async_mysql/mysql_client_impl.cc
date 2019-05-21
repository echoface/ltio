#include "mysql_client_impl.h"

namespace lt {

MysqlAsyncClientImpl::MysqlAsyncClientImpl(base::MessageLoop* loop, int count)
  : count_(count),
    loop_(loop) {
}

MysqlAsyncClientImpl::~MysqlAsyncClientImpl() {
  connections_.clear();
}

void MysqlAsyncClientImpl::InitWithOption(MysqlOptions& opt) {
  for (int i = 0; i < count_; i++) {
    RefMysqlAsyncConnect con(new MysqlAsyncConnect(this, loop_));
    con->InitConnection(opt);

    if (!con->SyncConnect()) {
      LOG(FATAL) << "mysql connect failed, host:" << opt.host << " port:" << opt.port;
      return;
    }
    connections_.push_back(std::move(con));
  }
}

void MysqlAsyncClientImpl::Close() {
  for (auto& con : connections_) {
    loop_->PostTask(NewClosure(std::bind(&MysqlAsyncConnect::SyncClose, con)));
  }
}

void MysqlAsyncClientImpl::PendingQuery(RefQuerySession& query, int timeout) {
  auto connection = get_client();
  if (!connection) {
    return;
  }
  connection->BindLoop()->PostTask(
    NewClosure(std::bind(&MysqlAsyncConnect::StartQuery, connection, query)));
}

MysqlAsyncConnect* MysqlAsyncClientImpl::get_client() {
  RefMysqlAsyncConnect con;
  uint32_t round_count = 0;
  do {
    use_index_++;
    round_count++;
    con = connections_[use_index_ % connections_.size()];
    if (con->IsReady()) {
      return con.get();
    }
  } while(!con && round_count < connections_.size());
  return NULL;
}

void MysqlAsyncClientImpl::OnConnectReady(MysqlAsyncConnect* con) {
}

void MysqlAsyncClientImpl::OnConnectionBroken(MysqlAsyncConnect* con) {

}

void MysqlAsyncClientImpl::OnConnectionClosed(MysqlAsyncConnect* con) {;

}

}//end lt
