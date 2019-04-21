#include <iostream>
#include <string>
#include "mysql_async_con.h"

const char* g_mysql_host="10.9.158.21";
const int   g_mysql_port=3306;
const char* g_mysql_user="fancy_test";
const char* g_mysql_pwd="fancy_test";
const char* g_mysql_database="mysql";

//const char* g_query_sql = "select User,Password from user where User!=''";
//const char* g_query_sql = "desc user";
//const char* g_query_sql = "select * from ftx_pre.order";
const char* g_query_sql = "use mysql";

MysqlConnection::MysqlConnection(MysqlClient* client, base::MessageLoop* bind_loop)
  : err_no_(0),
    client_(client),
    loop_(bind_loop) {
  current_state_ = CONNECT_IDLE;
}

//static
std::string MysqlConnection::MysqlWaitStatusString(int status) {
  std::ostringstream oss;
  if (status & MYSQL_WAIT_READ) oss << "READ,";
  if (status & MYSQL_WAIT_WRITE) oss << "WRITE,";
  if (status & MYSQL_WAIT_TIMEOUT) oss << "TIMEOUT,";
  if (status & MYSQL_WAIT_EXCEPT) oss << "EXCEPT,";
  return oss.str();
}

//static
int MysqlConnection::LtEvToMysqlStatus(base::LtEvent event) {

  int status = 0;
  if (event & base::LtEv::LT_EVENT_READ) {
    status |= MYSQL_WAIT_READ;
  }
  if (event & base::LtEv::LT_EVENT_WRITE) {
    status |= MYSQL_WAIT_WRITE;
  }
  if (event & base::LtEv::LT_EVENT_ERROR) {
    status |= MYSQL_WAIT_EXCEPT;
  }
/*
#define MYSQL_WAIT_EXCEPT 4
*/
  return status;
}

// return true when state can go next state imediately
bool MysqlConnection::go_next_state(int opt_status,
                                    const State wait_st,
                                    const State next_st) {
  if (opt_status) {
    this->current_state_ = next_st;
    return true;
  }

  // need to wait for data(add event to libevent)
  this->current_state_ = wait_st;

  WaitMysqlStatus(opt_status);

  return false;
}

void MysqlConnection::HandleState(int in_event) {

  bool st_continue = true;

  while(st_continue) {

    switch(current_state_) {

      case CONNECT_IDLE: {
        if (g_query_sql) {
          st_continue = true;
          current_state_ = QUERY_START;
        }
      } break;
      case QUERY_START: {

        int status = ::mysql_real_query_start(&err_no_,
                                              &mysql_,
                                              g_query_sql,
                                              (unsigned int)strlen(g_query_sql));

        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      } break;
      case QUERY_WAIT: {

        int status = ::mysql_real_query_cont(&err_no_, &mysql_, in_event);
        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      }break;
      case QUERY_RESULT_READY: {

        st_continue = true;
        if (err_no_ != 0) {
          current_state_ = CONNECT_DONE;
          LOG(ERROR) << "query error:" << ::mysql_error(&mysql_);
        } else {
          result_ = ::mysql_use_result(&mysql_);
          //TODO: when fail, go to CONNECT_DONE
          //current_state = (result_ != NULL) ? FETCH_ROW_START : QUERY_START;
          current_state_ = (result_ != NULL) ? FETCH_ROW_START : CONNECT_DONE;
          LOG_IF(ERROR, (result_ != NULL)) << "action query got null result";
        }
      } break;
      case FETCH_ROW_START: {

        int status = ::mysql_fetch_row_start(&result_row_, result_);
        st_continue = go_next_state(status, FETCH_ROW_WAIT, FETCH_ROW_RESULT_READY);
      }break;
      case FETCH_ROW_WAIT: {

        int status = mysql_fetch_row_cont(&result_row_, result_, in_event);
        st_continue = go_next_state(status, FETCH_ROW_WAIT, FETCH_ROW_RESULT_READY);
      }break;
      case FETCH_ROW_RESULT_READY: {

        LOG(INFO) << "FETCH_ROW_RESULT_READY enter";
        if (!result_row_) {
          if (::mysql_errno(&mysql_)) {
            printf("Error: %s\n", mysql_error(&mysql_));
          } else {
            /* EOF.no more rows */
            printf("EOF. no more result rows\n");
          }
          ::mysql_free_result(result_);

          //TODO: go to next query or to idle
          current_state_ = CONNECT_IDLE;
          break;
        }

        //TODO: add to result
        for (uint32_t i = 0; i < ::mysql_num_fields(result_); i++) {
          printf("%s%s", (i ? "\t | \t" : ""), (result_row_[i] ? result_row_[i] : "(null)"));
        }

        st_continue = go_next_state(0, FETCH_ROW_START, FETCH_ROW_START);
      } break;
      case CLOSE_START: {

        int status = mysql_close_start(&mysql_);
        st_continue = go_next_state(status, CLOSE_WAIT, CLOSE_DONE);
      }break;
      case CLOSE_WAIT: {

        int status= ::mysql_close_cont(&mysql_, in_event);
        st_continue = go_next_state(status,CLOSE_WAIT, CLOSE_DONE);
      } break;
      case CLOSE_DONE:

        st_continue = false;
        current_state_ = CONNECT_IDLE;
        break;
      default: {
        abort();
      }break;
    }
  };
}

void MysqlConnection::OnTimeOut() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  HandleState(MYSQL_WAIT_TIMEOUT);
}

void MysqlConnection::OnClose() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  int status = MysqlConnection::LtEvToMysqlStatus(fd_event_->ActivedEvent());
  HandleState(status);
}

void MysqlConnection::OnError() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  int status = MysqlConnection::LtEvToMysqlStatus(fd_event_->ActivedEvent());
  HandleState(status);
}

void MysqlConnection::OnWaitEventInvoked() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  int status = MysqlConnection::LtEvToMysqlStatus(fd_event_->ActivedEvent());
  HandleState(status);
}

void MysqlConnection::WaitMysqlStatus(int status) {
  LOG(INFO) << __FUNCTION__ << " wait event:" << MysqlConnection::MysqlWaitStatusString(status) << std::endl;

  base::EventPump* pump = base::MessageLoop::Current()->Pump();
  if (!fd_event_) {
    int fd = mysql_get_socket(&mysql_);
    fd_event_ = base::FdEvent::Create(fd, base::LtEv::LT_EVENT_NONE);

    fd_event_->SetErrorCallback(std::bind(&MysqlConnection::OnError, this));
    fd_event_->SetCloseCallback(std::bind(&MysqlConnection::OnClose, this));
    fd_event_->SetReadCallback(std::bind(&MysqlConnection::OnWaitEventInvoked, this));
    fd_event_->SetWriteCallback(std::bind(&MysqlConnection::OnWaitEventInvoked, this));

    pump->InstallFdEvent(fd_event_.get());
  }

  if (status & MYSQL_WAIT_READ) {
    fd_event_->EnableReading();
  } else {
    fd_event_->DisableReading();
  }

  if (status & MYSQL_WAIT_WRITE) {
    fd_event_->EnableWriting();
  } else {
    fd_event_->DisableWriting();
  }


  if (status & MYSQL_WAIT_TIMEOUT) {
    if (timeout_->IsAttached()) {
      pump->RemoveTimeoutEvent(timeout_.get());
    }
    int timeout = 1000 * mysql_get_timeout_value(&mysql_);
    timeout_->UpdateInterval(timeout);
    pump->AddTimeoutEvent(timeout_.get());
  }
}

void MysqlConnection::ConnectToServer() {
  if (!mysql_real_connect(&mysql_,
                           g_mysql_host,
                           g_mysql_user,
                           g_mysql_pwd,
                           g_mysql_database,
                           g_mysql_port, NULL, 0)) {
    LOG(INFO) << " connect to database failed" << ::mysql_error(&mysql_);
    return;
  }
  current_state_ = CONNECT_IDLE;
  LOG(INFO) << "mysql:" << g_mysql_host << " connected successful";
}

int main(int argc, char** argv) {
  mysql_library_init(0, NULL, NULL);

  base::MessageLoop loop;

  loop.SetLoopName("main");
  loop.Start();

  MysqlConnection* g_conn = new MysqlConnection(NULL, &loop);
  MysqlOptions option = {
    .host = "10.9.158.21",
    .port = 3306,
    .user = "fancy_test",
    .passwd = "fancy_test",
    .dbname = "mysql",
    .query_timeout = 5000,
  };
  g_conn->InitConnection(option);
  g_conn->ConnectToServer();

  loop.PostDelayTask(NewClosure([&]() {
    LOG(INFO) << "start query mysql db";
    g_conn->HandleState(0);
  }), 3000);

  loop.WaitLoopEnd();

  delete g_conn;
  mysql_library_end();
  return 0;
}

void MysqlConnection::InitConnection(const MysqlOptions& option) {
  ::mysql_init(&mysql_);
  option_ = option;

  uint32_t default_timeout = option.query_timeout / 1000; //second
  if (default_timeout < 1) {
    default_timeout = 1;
  }
  ::mysql_options(&mysql_, MYSQL_OPT_NONBLOCK, 0);
  ::mysql_options(&mysql_, MYSQL_READ_DEFAULT_GROUP, "async_queries");
  ::mysql_options(&mysql_, MYSQL_OPT_READ_TIMEOUT, &default_timeout);
  ::mysql_options(&mysql_, MYSQL_OPT_WRITE_TIMEOUT, &default_timeout);
  ::mysql_options(&mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &default_timeout);

  timeout_.reset(new base::TimeoutEvent(option.query_timeout, false));
  timeout_->InstallTimerHandler(NewClosure(std::bind(&MysqlConnection::OnTimeOut, this)));
}

