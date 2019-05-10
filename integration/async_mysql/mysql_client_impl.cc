#include "mysql_client_impl.h"

namespace lt {

MysqlAsyncClientImpl::MysqlAsyncClientImpl(base::MessageLoop* loop, int count)
  : count_(count),
    loop_(loop) {
}

MysqlAsyncClientImpl::~MysqlAsyncClientImpl() {
}

void MysqlAsyncClientImpl::InitWithOption(MysqlOptions& opt) {
  for (int i = 0; i < count_; i++) {
    RefMysqlAsyncConnect con(new MysqlAsyncConnect(this, loop_));
    con->InitConnection(opt);
    connections_.push_back(std::move(con));
  }
}

void MysqlAsyncClientImpl::PendingQuery(RefQuerySession& query, int timeout) {
  use_index_++;
  RefMysqlAsyncConnect con = connections_[use_index_ % connections_.size()];
  con->BindLoop()->PostTask(NewClosure(std::bind(&MysqlAsyncConnect::StartQuery, con, query)));
}

void MysqlAsyncClientImpl::OnQueryFinish(RefQuerySession query) {
  query->OnQueryDone();
}

void MysqlAsyncClientImpl::OnConnectionBroken(MysqlAsyncConnect* con) {
  //auto iter = std::find(connections_.begin(), connections_.end(), con);
  //if (iter != connections_.end()) {
    //connections_.erase(iter);
  //}
}

}//end lt
