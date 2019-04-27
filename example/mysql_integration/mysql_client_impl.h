#ifndef _LT_MYSQL_ASYNC_CLIENT_IMPL_H_H
#define _LT_MYSQL_ASYNC_CLIENT_IMPL_H_H

#include "mysql_async_con.h"
#include "query_session.h"

typedef std::shared_ptr<MysqlConnection> RefMysqlConnection;

class MysqlAsyncClientImpl : public MysqlClient,
                             public QuerySession::QueryActor {
  public:
    MysqlAsyncClientImpl();
    ~MysqlAsyncClientImpl();

    RefQuerySession StartQuery();
    void PendingQuery(RefQuerySession& query);
    void ConnectionBroken(MysqlConnection* con) override;
  private:
    std::vector<RefMysqlConnection> connections_;
};

#endif
