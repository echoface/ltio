#ifndef _LT_MYSQL_QUERY_REQUEST_H_H
#define _LT_MYSQL_QUERY_REQUEST_H_H

#include <list>
#include <vector>
#include <string>
#include <memory>

#include <mysql.h>
#include "base/closure/closure_task.h"
#include "base/message_loop/message_loop.h"

class QuerySession;
class MysqlConnection;
class MysqlAsyncClientImpl;

typedef std::vector<std::string> ResultRow;
typedef std::shared_ptr<QuerySession> RefQuerySession;

class QuerySession {
  public:
    typedef struct {
      virtual void PendingQuery(RefQuerySession& query);
    } QueryActor;
    ~QuerySession();

    QuerySession& UseDB(const std::string& db);
    QuerySession& Query(const std::string& sql);
    QuerySession& Then(base::StlClosure callback);
    void Do();

    const char* DB() const {return db_name_.c_str()};
    const std::string& QueryContent() const {return query_;}
  private:
    friend class MysqlConnection;
    friend class MysqlAsyncClientImpl;
    QuerySession(QueryActor* actor);
    void SetCode(int code, std::string& err_message);

    QueryActor* actor_;
    int code_;
    uint32_t affected_rows_;
    std::string error_message_;

    std::string query_;
    std::string db_name_;

    std::vector<ResultRow> results_;
    std::vector<std::string> colum_names_;
    base::StlClosure finish_callback_;
};

#endif
