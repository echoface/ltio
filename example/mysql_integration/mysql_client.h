#ifndef _LT_MYSQL_ASYNC_CONNECTION_H_H
#define _LT_MYSQL_ASYNC_CONNECTION_H_H

#include <string>
#include <mysql/mysql.h>
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
  static int LtEvToMysqlStatus(base::LtEvent event);
  static std::string MysqlWaitStatusString(int status);
  struct MysqlClient {
    virtual void ConnectionBroken(MysqlConnection* con);
  };

  MysqlConnection(MysqlClient* client, base::MessageLoop* bind_loop);

  void InitConnection(const MysqlOptions& option);
  void ConnectToServer();

  void HandleState(int status = 0);
private:
  enum State{
    QUERY_START,
    QUERY_CONTINUE,

    START_USE_RESULT,

    FETCH_ROW_START,
    FETCH_ROW_CONTINUE,
    FETCH_ROW_RESULT_READY,

    CONNECTION_IDLE
  };

  void OnError();
  void OnClose();
  void OnTimeOut();
  void OnWaitEventInvoked();

  void WaitMysqlStatus(int status);

  bool HandleQueryStart(int in_event);
  bool HandleQueryContinue(int in_event);
private:
  int err_no_;
  int current_state_;                   // State machine current state

  MYSQL mysql_;
  MYSQL *mysql_ret = NULL;

  MYSQL_RES* result_ = NULL;
  //typedef char** MYSQL_ROW
  MYSQL_ROW result_row_ = NULL;

  MysqlOptions option_;
  MysqlClient* client_ = NULL;
  base::MessageLoop* loop_ = NULL;
  base::RefFdEvent fd_event_;
  std::unique_ptr<base::TimeoutEvent> timeout_;
};

#endif
