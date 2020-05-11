#include "query_session.h"

namespace lt {
//static
RefQuerySession QuerySession::New() {
  RefQuerySession query_session(new QuerySession());
  return query_session;
}

QuerySession::QuerySession() {
}

QuerySession::~QuerySession() {
}

QuerySession& QuerySession::UseDB(const std::string& db) {
  db_name_ = db;
  return *this;
}
QuerySession& QuerySession::Query(const std::string& sql) {
  query_ = sql;
  return *this;
}
QuerySession& QuerySession::Then(StlClosure callback) {
  finish_callback_ = callback;
  return *this;
}

void QuerySession::SetCode(int code, std::string& err_message) {
  code_ = code;
  err_message_ = err_message;
}

void QuerySession::SetCode(int code, const char* err_message) {
  code_ = code;
  err_message_ = err_message;
}

void QuerySession::PendingRow(ResultRow&& one_row) {
  results_.push_back(std::move(one_row));
}

void QuerySession::OnQueryDone() {
  if (finish_callback_) {
    finish_callback_();
  }
  finish_callback_ = nullptr;
}

}//end lt

