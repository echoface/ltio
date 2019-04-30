#include "mysql_client_impl.h"

MysqlAsyncClientImpl::MysqlAsyncClientImpl(base::MessageLoop* loop, int count)
  : count_(count),
    loop_(loop) {
}

MysqlAsyncClientImpl::~MysqlAsyncClientImpl() {
}

void MysqlAsyncClientImpl::InitWithOption(MysqlOptions& opt) {
  for (int i = 0; i < count_; i++) {
    RefMysqlConnection con(new MysqlConnection(this, loop_));
    con->InitConnection(opt);
    connections_.push_back(std::move(con));
  }
}

void MysqlAsyncClientImpl::PendingQuery(RefQuerySession& query, int timeout) {
  use_index_++;
  RefMysqlConnection con = connections_[use_index_ % connections_.size()];
  con->BindLoop()->PostTask(NewClosure(std::bind(&MysqlConnection::StartQuery, con, query)));
}

void MysqlAsyncClientImpl::OnQueryFinish(RefQuerySession query) {
  query->OnQueryDone();
}

void MysqlAsyncClientImpl::OnConnectionBroken(MysqlConnection* con) {
  //auto iter = std::find(connections_.begin(), connections_.end(), con);
  //if (iter != connections_.end()) {
    //connections_.erase(iter);
  //}
}
