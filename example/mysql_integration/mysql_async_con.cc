#include <iostream>
#include <string>
#include "mysql_async_con.h"

namespace lt {

MysqlAsyncConnect::MysqlAsyncConnect(MysqlClient* client, base::MessageLoop* bind_loop)
  : err_no_(0),
    client_(client),
    loop_(bind_loop) {
  current_state_ = CONNECT_IDLE;
}

//static
std::string MysqlAsyncConnect::MysqlWaitStatusString(int status) {
  std::ostringstream oss;
  if (status == 0) oss << "None";

  if (status & MYSQL_WAIT_READ) oss << "READ|";
  if (status & MYSQL_WAIT_WRITE) oss << "WRITE|";
  if (status & MYSQL_WAIT_TIMEOUT) oss << "TIMEOUT|";
  if (status & MYSQL_WAIT_EXCEPT) oss << "EXCEPT|";
  return oss.str();
}

//static
int MysqlAsyncConnect::LtEvToMysqlStatus(base::LtEvent event) {

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
bool MysqlAsyncConnect::go_next_state(int opt_status,
                                    const State wait_st,
                                    const State next_st) {
  VLOG(1) << "wait:" << MysqlWaitStatusString(opt_status)
    << " wait_st:" << wait_st
    << " next_st:" << next_st;

  if (opt_status == 0) {
    current_state_ = next_st;
    return true;
  }

  // need to wait for data(add event to libevent)
  current_state_ = wait_st;

  WaitMysqlStatus(opt_status);

  return false;
}

void MysqlAsyncConnect::HandleState(int in_event) {
  bool st_continue = true;

  VLOG_IF(2, in_event) << " event:" << MysqlWaitStatusString(in_event);

  while(st_continue) {
    VLOG(1) << "go state:" << current_state_;
    switch(current_state_) {
      case CONNECT_IDLE: {
        st_continue = query_list_.size() > 0;
        current_state_ = st_continue ? QUERY_START : CONNECT_IDLE;
      } break;
      case CONNECT_START: {
        int status = mysql_real_connect_start(&mysql_ret_, &mysql_,
                                              option_.host.c_str(),
                                              option_.user.c_str(),
                                              option_.passwd.c_str(),
                                              option_.dbname.c_str(),
                                              option_.port, NULL, 0);

        st_continue = go_next_state(status, CONNECT_WAIT, CONNECT_DONE);
      }break;
      case CONNECT_WAIT: {
        int status = mysql_real_connect_cont(&mysql_ret_, &mysql_, in_event);
        st_continue = go_next_state(status, CONNECT_WAIT, CONNECT_DONE);
      }break;
      case CONNECT_DONE: {
        if (!mysql_ret_)  {
          LOG(FATAL) << " connect failed, host:" << option_.host;
        }
        st_continue = go_next_state(0, CONNECT_IDLE, CONNECT_IDLE);
        last_selected_db_ = option_.dbname;
        LOG(INFO) << "connect to " << option_.host << "success";
      }break;
      case QUERY_START: {
        CHECK(!in_process_query_ && query_list_.size());

        in_process_query_ = query_list_.front();
        query_list_.pop_front();

        if (!in_process_query_->DB().empty() &&
            last_selected_db_ != in_process_query_->DB()) {
          ::mysql_select_db(&mysql_, in_process_query_->DB().c_str());
        }

        const std::string& content = in_process_query_->QueryContent();

        int status = ::mysql_real_query_start(&err_no_,
                                              &mysql_,
                                              content.c_str(),
                                              content.size());
        if (err_no_ != 0) {
          in_process_query_->SetCode(err_no_, ::mysql_error(&mysql_));
          LOG(INFO) << "query start failed:" << content << " err:" << ::mysql_error(&mysql_);
          FinishCurrentQuery(CONNECT_IDLE);
          st_continue = true;
          break;
        }

        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      } break;
      case QUERY_WAIT: {
        int status = ::mysql_real_query_cont(&err_no_, &mysql_, in_event);
        if (err_no_ != 0) {
          LOG(INFO) << "query continue failed:" << in_event << " err:" <<  ::mysql_error(&mysql_);
          in_process_query_->SetCode(err_no_, ::mysql_error(&mysql_));
          FinishCurrentQuery(CONNECT_IDLE);
          st_continue = true;
          break;
        }

        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      }break;
      case QUERY_RESULT_READY: {
        CHECK(in_process_query_);

        result_ = ::mysql_use_result(&mysql_);
        if (result_ == NULL) {
          std::string error_message(mysql_error(&mysql_));
          in_process_query_->SetCode(err_no_, error_message);

          FinishCurrentQuery(CONNECT_IDLE);
          st_continue = true;
          break;
        }

        st_continue = go_next_state(0, FETCH_ROW_START, FETCH_ROW_START);
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
        CHECK(in_process_query_);

        if (!result_row_) {
          if (::mysql_errno(&mysql_)) {
            std::string error_message(mysql_error(&mysql_));
            in_process_query_->SetCode(err_no_, error_message);
          }
          //else mean /* EOF.no more rows */
          in_process_query_->SetResultRows(::mysql_num_rows(result_));
          ::mysql_free_result(result_);

          FinishCurrentQuery(CONNECT_IDLE);
          st_continue = true;
          break;
        }

        auto& heards = in_process_query_->MutableHeaders();
        if (heards.empty()) {
          ParseResultDesc(result_, in_process_query_.get());
        }

        ResultRow row;
        uint32_t field_count = ::mysql_num_fields(result_);
        for (uint32_t i = 0; i < field_count; i++) {
          if (result_row_[i]) {
            row.push_back(result_row_[i]);
          } else {
            row.push_back("NULL");
          }
        }
        in_process_query_->PendingRow(std::move(row));

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

bool MysqlAsyncConnect::ParseResultDesc(MYSQL_RES* result, QuerySession* query) {
  auto& heards = query->MutableHeaders();

  MYSQL_FIELD *field_desc;
  //uint32_t field_count = ::mysql_num_fields(result);
  field_desc = ::mysql_fetch_field(result_);
  while(field_desc != NULL) {
    heards.push_back(field_desc->name);
    field_desc = ::mysql_fetch_field(result_);
  }
  return true;
}

void MysqlAsyncConnect::FinishCurrentQuery(State next_st) {
  CHECK(in_process_query_);
  in_process_query_->SetAffectedRows(::mysql_affected_rows(&mysql_));

  client_->OnQueryFinish(in_process_query_);

  in_process_query_.reset();
  current_state_ = next_st;
  err_no_ = 0;
}

void MysqlAsyncConnect::OnTimeOut() {
  reset_wait_event();
  HandleState(MYSQL_WAIT_TIMEOUT);
}

void MysqlAsyncConnect::OnClose() {
  int status = LtEvToMysqlStatus(fd_event_->ActivedEvent());
  reset_wait_event();
  HandleState(status);
}

void MysqlAsyncConnect::OnError() {
  int status = LtEvToMysqlStatus(fd_event_->ActivedEvent());
  reset_wait_event();

  HandleState(status);
}

void MysqlAsyncConnect::OnWaitEventInvoked() {
  int status = LtEvToMysqlStatus(fd_event_->ActivedEvent());
  reset_wait_event();
  HandleState(status);
}

void MysqlAsyncConnect::reset_wait_event() {
  base::EventPump* pump = loop_->Pump();
  pump->RemoveTimeoutEvent(timeout_.get());
  fd_event_->DisableAll();
}

void MysqlAsyncConnect::WaitMysqlStatus(int status) {

  base::EventPump* pump = base::MessageLoop::Current()->Pump();
  if (!fd_event_) {
    int fd = mysql_get_socket(&mysql_);
    fd_event_ = base::FdEvent::Create(fd, base::LtEv::LT_EVENT_NONE);

    fd_event_->SetErrorCallback(std::bind(&MysqlAsyncConnect::OnError, this));
    fd_event_->SetCloseCallback(std::bind(&MysqlAsyncConnect::OnClose, this));
    fd_event_->SetReadCallback(std::bind(&MysqlAsyncConnect::OnWaitEventInvoked, this));
    fd_event_->SetWriteCallback(std::bind(&MysqlAsyncConnect::OnWaitEventInvoked, this));

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

void MysqlAsyncConnect::StartQuery(RefQuerySession& query) {
  CHECK(loop_->IsInLoopThread());

  query_list_.push_back(query);
  if (current_state_ == State::CONNECT_IDLE) {
    HandleState(0);
  }
}

void MysqlAsyncConnect::InitConnection(const MysqlOptions& option) {
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
  timeout_->InstallTimerHandler(NewClosure(std::bind(&MysqlAsyncConnect::OnTimeOut, this)));

  current_state_ = CONNECT_START;

  /*
  if (loop_->IsInLoopThread()) {
    return HandleState(0);
  }*/
  loop_->PostTask(NewClosure(std::bind(&MysqlAsyncConnect::HandleState, this, 0)));
}

}//end lt

