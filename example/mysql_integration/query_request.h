#ifndef _LT_MYSQL_QUERY_REQUEST_H_H
#define _LT_MYSQL_QUERY_REQUEST_H_H

#include <string>
#include <mysql.h>
#include "base/message_loop/message_loop.h"

class QueryRequest {
  public:
    QueryRequest();
    ~QueryRequest();

    QueryRequest& UseDB(const std::string& db_name);
    QueryRequest& Query(const std::string& sql);
    QueryRequest& OnError()

  private:
};

#endif
