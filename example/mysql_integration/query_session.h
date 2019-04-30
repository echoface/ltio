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

struct ResultDesc {
  uint32_t affected_rows_ = 0;
  std::vector<std::string> colum_names_;
};
typedef std::unique_ptr<ResultDesc> ResultDescPtr;

class QuerySession {
  public:
    static RefQuerySession New();
    ~QuerySession();

    QuerySession& UseDB(const std::string& db);
    QuerySession& Query(const std::string& sql);
    QuerySession& Then(base::StlClosure callback);

    const int Code() const {return code_;}
    const std::string& ErrorMessage() const {return err_message_;}
    const ResultDesc* RawResultDesc() const {return desc_.get();}
    const std::vector<ResultRow>& Result() const {return results_;}
  private:
    friend class MysqlConnection;
    friend class MysqlAsyncClientImpl;

    QuerySession();

    void OnQueryDone();

    const std::string& DB() const {return db_name_;};
    const std::string& QueryContent() const {return query_;}

    void PendingRow(ResultRow&& one_row);
    void SetResultDesc(ResultDescPtr desc);
    void SetCode(int code, const char* err_message);
    void SetCode(int code, std::string& err_message);

    int code_ = 0;
    std::string err_message_;

    std::string query_;
    std::string db_name_;

    ResultDescPtr desc_;

    std::vector<ResultRow> results_;
    base::StlClosure finish_callback_;
};
#endif
