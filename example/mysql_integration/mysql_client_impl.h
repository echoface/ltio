#ifndef _LT_MYSQL_ASYNC_CLIENT_IMPL_H_H
#define _LT_MYSQL_ASYNC_CLIENT_IMPL_H_H

#include <list>
#include <vector>
#include "mysql_async_con.h"
#include "query_session.h"

namespace lt {

typedef std::shared_ptr<MysqlAsyncConnect> RefMysqlAsyncConnect;

//root:passwd@192.168.1.1:3306?db=&timeout=&charset=

class MysqlAsyncClientImpl : public MysqlAsyncConnect::MysqlClient {
  public:
    MysqlAsyncClientImpl(base::MessageLoop* loop, int count);
    ~MysqlAsyncClientImpl();

    void InitWithOption(MysqlOptions& opt);

    void PendingQuery(RefQuerySession& query, int timeout = 0);

    void OnQueryFinish(RefQuerySession query) override;
    void OnConnectionBroken(MysqlAsyncConnect* con) override;
  private:
    const int count_ = 4;
    base::MessageLoop* loop_ = NULL;
    std::atomic<int> use_index_;
    std::vector<RefMysqlAsyncConnect> connections_;
};


}
#endif
