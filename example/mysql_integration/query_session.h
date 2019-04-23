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
typedef std::vector<std::string> ResultRow;
typedef std::shared_ptr<QuerySession> RefQuerySession;

class QuerySession {
  public:
    typedef struct {
      virtual void PendingQuery()
    } QueryActor;
    ~QuerySession();

    QuerySession& UseDB(const std::string& db);
    QuerySession& Query(const std::string& sql);
    QuerySession& Then(base::StlClosure callback);
    void Do();
  private:
    QuerySession(QueryActor* actor);

    int code_;
    uint32_t affected_rows_;
    std::string error_message_;
    std::vector<ResultRow> results_;
    std::vector<std::string> colum_names_;
    base::StlClosure finish_callback_;
};

#endif
