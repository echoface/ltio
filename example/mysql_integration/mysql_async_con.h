#ifndef _LT_MYSQL_ASYNC_CONNECTION_H_H
#define _LT_MYSQL_ASYNC_CONNECTION_H_H

#include <list>
#include <string>
#include <mysql.h>
#include "query_session.h"
#include "base/message_loop/message_loop.h"

struct MysqlOptions {
  std::string host;
  uint32_t port;
  std::string user;
  std::string passwd;
  std::string dbname;
  uint32_t query_timeout;
};

class MysqlConnection {
public:
  enum State{
    CONNECT_IDLE,
    CONNECT_START,
    CONNECT_WAIT,
    CONNECT_DONE,
    QUERY_START,
    QUERY_WAIT,
    QUERY_RESULT_READY,
    FETCH_ROW_START,
    FETCH_ROW_WAIT,
    FETCH_ROW_RESULT_READY,
    CLOSE_START,
    CLOSE_WAIT,
    CLOSE_DONE,
  };

  static int LtEvToMysqlStatus(base::LtEvent event);
  static std::string MysqlWaitStatusString(int status);

  struct MysqlClient {
    virtual void ConnectionBroken(MysqlConnection* con);
  };

  MysqlConnection(MysqlClient* client, base::MessageLoop* bind_loop);

  void StartAQuery(const char* query);
  void StartQuery(RefQuerySession& query);

  void InitConnection(const MysqlOptions& option);
  void ConnectToServer();

  void HandleState(int status = 0);
  base::MessageLoop* BindLoop() {return loop_;}
private:
  void OnError();
  void OnClose();
  void OnTimeOut();
  void OnWaitEventInvoked();

  void reset_wait_event();
  void WaitMysqlStatus(int status);
  bool go_next_state(int status, const State wait_st, const State next_st);
private:


  int err_no_;
  int current_state_;                   // State machine current state

  MYSQL mysql_;
  MYSQL *mysql_ret_ = NULL;

  MYSQL_RES* result_ = NULL;
  //typedef char** MYSQL_ROW
  MYSQL_ROW result_row_ = NULL;

  MysqlOptions option_;
  MysqlClient* client_ = NULL;
  base::MessageLoop* loop_ = NULL;
  base::RefFdEvent fd_event_;
  std::unique_ptr<base::TimeoutEvent> timeout_;

  std::string last_selected_db_;
  RefQuerySession in_process_query_;
  std::list<RefQuerySession> query_list_;
};

#endif
