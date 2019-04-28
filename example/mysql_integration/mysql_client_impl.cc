#include "mysql_client_impl.h"

MysqlAsyncClientImpl::MysqlAsyncClientImpl(base::MessageLoop* loop, int count)
  : count_(count),
    loop_(loop) {
}

MysqlAsyncClientImpl::~MysqlAsyncClientImpl() {
}

RefQuerySession MysqlAsyncClientImpl::StartQuery() {
  RefQuerySession query(new QuerySession(this));
  return query;
}

void MysqlAsyncClientImpl::InitWithOption(MysqlOptions& opt) {
  for (int i = 0; i < count_; i++) {
    RefMysqlConnection con(new MysqlConnection(this, loop_));
    con->InitConnection(opt);
    connections_.push_back(std::move(con));
  }
}

void MysqlAsyncClientImpl::PendingQuery(RefQuerySession& query) {
  use_index_++;
  RefMysqlConnection con = connections_[use_index_ % connections_.size()];
  con->BindLoop()->PostTask(NewClosure(std::bind(MysqlConnection::StartQuery, con, query)));
}

void MysqlAsyncClientImpl::ConnectionBroken(MysqlConnection* con) {
  auto iter = std::find(connections_.begin(), connections_.end(), con);
  if (iter != connections_.end()) {
    connections_.erase(iter);
  }
}
