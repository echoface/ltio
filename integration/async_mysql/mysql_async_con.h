#ifndef _LT_MYSQL_ASYNC_CONNECTION_H_H
#define _LT_MYSQL_ASYNC_CONNECTION_H_H

#include <list>
#include <string>
#include <mysql.h>
#include "query_session.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/repeating_timer.h"

namespace lt {

struct MysqlOptions {
  std::string host;
  uint32_t port;
  std::string user;
  std::string passwd;
  std::string dbname;
  uint32_t query_timeout;
  bool auto_re_connect = false;
};

typedef std::shared_ptr<base::TimeoutEvent> RefTimeoutEvent;

class MysqlAsyncConnect {
public:
  enum State{
    CONNECT_INIT,
    CONNECT_START,
    CONNECT_WAIT,
    CONNECT_DONE,

    SELECT_DB_START,
    SELECT_DB_WAIT,
    SELECT_DB_DONE,

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
    virtual ~MysqlClient() {}
    virtual void OnConnectReady(MysqlAsyncConnect* con) {};
    virtual void OnConnectionClosed(MysqlAsyncConnect* con) {};
    virtual void OnConnectionBroken(MysqlAsyncConnect* con) {};
  };

  MysqlAsyncConnect(MysqlClient* client, base::MessageLoop* bind_loop);
  ~MysqlAsyncConnect();

  void StartQuery(RefQuerySession& query);

  void ResetClient() {client_ = NULL;};
  void InitConnection(const MysqlOptions& option);
  base::MessageLoop* BindLoop() {return loop_;}


  void Connect();
  bool SyncConnect();

  void Close();
  bool SyncClose();

  bool IsReady() {return ready_;}
private:
  void FinishCurrentQuery(State next_st);

  void HandleState(int in_event = 0);
  void HandleStateConnect(int in_event = 0);
  void HandleStateSelectDB(int in_event = 0);

  void OnError();
  void OnClose();
  void OnTimeOut();
  void OnWaitEventInvoked();
  void WaitMysqlStatus(int status);

  void clean_up();
  void reset_wait_event();
  void do_connection_check();
  bool go_next_state(int status, const State wait_st, const State next_st);

  bool ParseResultDesc(MYSQL_RES* result, QuerySession* query);
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
  RefTimeoutEvent timeout_;
  base::RepeatingTimer checker_;

  bool ready_ = false;
  bool schedule_close_ = false;

  std::string last_selected_db_;
  RefQuerySession in_process_;
  std::list<RefQuerySession> query_list_;
};

}
#endif
