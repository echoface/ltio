#include <iostream>
#include <string>
#include "mysql_async_con.h"

MysqlConnection::MysqlConnection(MysqlClient* client, base::MessageLoop* bind_loop)
  : err_no_(0),
    client_(client),
    loop_(bind_loop) {
  current_state_ = CONNECT_IDLE;
}

//static
std::string MysqlConnection::MysqlWaitStatusString(int status) {
  std::ostringstream oss;
  if (status == 0) oss << "None";

  if (status & MYSQL_WAIT_READ) oss << "READ|";
  if (status & MYSQL_WAIT_WRITE) oss << "WRITE|";
  if (status & MYSQL_WAIT_TIMEOUT) oss << "TIMEOUT|";
  if (status & MYSQL_WAIT_EXCEPT) oss << "EXCEPT|";
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
  LOG(INFO) << "go next, wait_option:" << MysqlWaitStatusString(opt_status)
    << " wait_st:" << wait_st << " next_st:" << next_st;

  if (opt_status == 0) {
    current_state_ = next_st;
    return true;
  }

  // need to wait for data(add event to libevent)
  current_state_ = wait_st;

  WaitMysqlStatus(opt_status);

  return false;
}

void MysqlConnection::StartAQuery(const char* query) {
  next_query_ = query;
  LOG(INFO) << " new next_query: " << next_query_;
  if (current_state_ == CONNECT_IDLE) {
    HandleState(0);
  }
}

void MysqlConnection::HandleState(int in_event) {
  LOG(INFO) << " MysqlConnection::HandleState enter, " << current_state_;

  bool st_continue = true;

  while(st_continue) {

    switch(current_state_) {
      case CONNECT_IDLE: {
        st_continue = next_query_.size() > 0;
        current_state_ = st_continue ? QUERY_START : CONNECT_IDLE;
        LOG(INFO) << " connect idle go next:" << current_state_ << " next query:" << next_query_;
      } break;
      case CONNECT_START: {
        LOG(INFO) << " connect start";
        int status = mysql_real_connect_start(&mysql_ret_, &mysql_,
                                              option_.host.c_str(),
                                              option_.user.c_str(),
                                              option_.passwd.c_str(),
                                              option_.dbname.c_str(),
                                              option_.port, NULL, 0);

        st_continue = go_next_state(status, CONNECT_WAIT, CONNECT_DONE);
      }break;
      case CONNECT_WAIT: {
        LOG(INFO) << " connect wait";

        int status = mysql_real_connect_cont(&mysql_ret_, &mysql_, in_event);
        st_continue = go_next_state(status, CONNECT_WAIT, CONNECT_DONE);
      }break;
      case CONNECT_DONE: {
        LOG(INFO) << " connect done";
        if (!mysql_ret_)  {
          LOG(FATAL) << " connect failed, host:" << option_.host;
        }
        st_continue = go_next_state(0, CONNECT_IDLE, CONNECT_IDLE);
        LOG(INFO) << " connect mysql success:" << option_.host;
        last_selected_db_ = option_.dbname;
      }break;
      case QUERY_START: {
        LOG(INFO) << " query start";
        CHECK(!in_process_query_ && query_list_.size());

        in_process_query_ = query_list_.back(); 
        query_list_.pop_back();

        if (last_selected_db_ != in_process_query_->DB()) {
          ::mysql_select_db(&mysql_, in_process_query_->DB()); 
        }

        const std::string& content = in_process_query_->QueryContent();

        int status = ::mysql_real_query_start(&err_no_,
                                              &mysql_,
                                              content.c_str(),
                                              content.size());

        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      } break;
      case QUERY_WAIT: {
        LOG(INFO) << " query wait";
        int status = ::mysql_real_query_cont(&err_no_, &mysql_, in_event);
        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      }break;
      case QUERY_RESULT_READY: {
        LOG(INFO) << " query result ready";
        CHECK(in_process_query_);

        if (err_no_ != 0) {
          st_continue = true;
          current_state_ = CONNECT_DONE;
          LOG(ERROR) << "query error:" << ::mysql_error(&mysql_);
          in_process_query_->SetCode(err_no_, ::mysql_error(&mysql_));
        } else {
          result_ = ::mysql_use_result(&mysql_);
          LOG_IF(ERROR, (result_ == NULL)) << "action query got null result";

          //TODO: when fail, go to CONNECT_DONE
          State next_st = (result_ != NULL) ? FETCH_ROW_START : CLOSE_START;
          st_continue = go_next_state(0, next_st, next_st);
        }
      } break;
      case FETCH_ROW_START: {
        LOG(INFO) << " fetch result start";

        int status = ::mysql_fetch_row_start(&result_row_, result_);
        st_continue = go_next_state(status, FETCH_ROW_WAIT, FETCH_ROW_RESULT_READY);
      }break;
      case FETCH_ROW_WAIT: {
        LOG(INFO) << " fetch result wait";

        int status = mysql_fetch_row_cont(&result_row_, result_, in_event);
        st_continue = go_next_state(status, FETCH_ROW_WAIT, FETCH_ROW_RESULT_READY);
      }break;
      case FETCH_ROW_RESULT_READY: {
        LOG(INFO) << " handle fetch row resut ready";

        if (!result_row_) {
          if (::mysql_errno(&mysql_)) {
            LOG(ERROR) << "fetch result error:" << mysql_error(&mysql_);
          } else { /* EOF.no more rows */
            LOG(INFO) << "EOF. no more result rows";
            next_query_.clear();
          }
          //TODO: go to next query or to idle

          ::mysql_free_result(result_);

          st_continue = false;
          current_state_ = CONNECT_IDLE;
          break;
        }

        uint32_t field_count = ::mysql_num_fields(result_);

        std::ostringstream oss;
        for (uint32_t i = 0; i < field_count; i++) {
          oss << (result_row_[i] ? result_row_[i] : "(null)");
          if (i != field_count - 1) {
            oss << "\t|\t";
          }
        }
        LOG(INFO) << oss.str();

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
  reset_wait_event();
  LOG(INFO) << __FUNCTION__ << " event back:" << MysqlWaitStatusString(MYSQL_WAIT_TIMEOUT);

  HandleState(MYSQL_WAIT_TIMEOUT);
}

void MysqlConnection::OnClose() {
  int status = MysqlConnection::LtEvToMysqlStatus(fd_event_->ActivedEvent());
  LOG(INFO) << __FUNCTION__ << " event back:" << MysqlWaitStatusString(status);

  reset_wait_event();

  HandleState(status);
}

void MysqlConnection::OnError() {
  int status = MysqlConnection::LtEvToMysqlStatus(fd_event_->ActivedEvent());
  LOG(INFO) << __FUNCTION__ << " event back:" << MysqlWaitStatusString(status);
  reset_wait_event();

  HandleState(status);
}

void MysqlConnection::OnWaitEventInvoked() {
  int status = MysqlConnection::LtEvToMysqlStatus(fd_event_->ActivedEvent());
  LOG(INFO) << __FUNCTION__ << " event back:" << MysqlWaitStatusString(status);
  reset_wait_event();

  HandleState(status);
}

void MysqlConnection::reset_wait_event() {
  base::EventPump* pump = loop_->Pump();
  pump->RemoveTimeoutEvent(timeout_.get());

  fd_event_->DisableAll();
}

void MysqlConnection::WaitMysqlStatus(int status) {

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
                          option_.host.c_str(),
                          option_.user.c_str(),
                          option_.passwd.c_str(),
                          option_.dbname.c_str(),
                          option_.port, NULL, 0)) {
    LOG(INFO) << " connect to database failed" << ::mysql_error(&mysql_);
    return;
  }
  current_state_ = CONNECT_IDLE;
  LOG(INFO) << "mysql:" << option_.host << " connected successful";
}

void MysqlConnection::StartQuery(RefQuerySession& query) {
  CHECK(loop_->IsInLoopThread());

  query_list_.push_back(query);
  if (current_state_ == State::CONNECT_IDLE) {
    HandleState(0);
  }
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

  current_state_ = CONNECT_START;

  /*
  if (loop_->IsInLoopThread()) {
    return HandleState(0);
  }*/
  loop_->PostTask(NewClosure(std::bind(&MysqlConnection::HandleState, this, 0)));
}

