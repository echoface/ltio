#ifndef _LT_MYSQL_ASYNC_CLIENT_IMPL_H_H
#define _LT_MYSQL_ASYNC_CLIENT_IMPL_H_H

#include <list>
#include <vector>
#include "mysql_async_con.h"
#include "query_session.h"

typedef std::shared_ptr<MysqlConnection> RefMysqlConnection;

//root:passwd@192.168.1.1:3306?db=&timeout=&charset=

class MysqlAsyncClientImpl : public MysqlClient,
                             public QuerySession::QueryActor {
  public:
    MysqlAsyncClientImpl(base::MessageLoop* loop, int count);
    ~MysqlAsyncClientImpl();

    RefQuerySession StartQuery();

    void InitWithOption(MysqlOptions& opt);

    void PendingQuery(RefQuerySession& query);
    void ConnectionBroken(MysqlConnection* con) override;
  private:
    const int count_ = 4;
    base::MessageLoop* loop_ = NULL;
    std::atomic<int> use_index_;
    std::vector<RefMysqlConnection> connections_;
};

#endif
