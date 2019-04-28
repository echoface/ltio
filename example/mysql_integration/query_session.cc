#include "query_session.h"

QuerySession::QuerySession(QueryActor* actor)
: actor_(actor) {
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
QuerySession& QuerySession::Then(base::StlClosure callback) {
  return *this;
}

void QuerySession::Do() {
}

void QuerySession::SetCode(int code, std::string& err_message) {
  code_ = code;
  error_message_ = err_message;
}
