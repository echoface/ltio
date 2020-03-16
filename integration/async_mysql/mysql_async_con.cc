#include <iostream>
#include <string>
#include "mysql_async_con.h"

namespace lt {

#define ERR_STR ::mysql_error(&mysql_)

MysqlAsyncConnect::MysqlAsyncConnect(MysqlClient* client, base::MessageLoop* bind_loop)
  : client_(client),
    loop_(bind_loop),
    checker_(bind_loop) {
}

MysqlAsyncConnect::~MysqlAsyncConnect() {
  if (in_process_) {
    in_process_->SetCode(-1, "closed");
    in_process_->OnQueryDone();
    in_process_.reset();
  }

  for (auto& query : query_list_) {
    query->SetCode(-1, "closed");
    query->OnQueryDone();
  }
  query_list_.clear();

  if (timeout_ || fd_event_) {
    RefTimeoutEvent to = timeout_;
    base::RefFdEvent ev = fd_event_;
    auto clean_fn = [ev, to]() {
      auto pump = base::MessageLoop::Current()->Pump();
      if (ev) pump->RemoveFdEvent(ev.get());
      if (to) pump->RemoveTimeoutEvent(to.get());
    };
    loop_->PostTask(NewClosure(clean_fn));
  }
  checker_.Stop();
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

// select db as part of query
bool MysqlAsyncConnect::HandleStateSelectDB(int in_event) {
  LOG(ERROR) << in_event << " cur st:" << current_state_;
  CHECK(in_process_);

  switch(current_state_) {
    case SELECT_DB_START: {
      const char* db = in_process_->DB().c_str();

      int error_no = 0;
      int status = ::mysql_select_db_start(&error_no, &mysql_, db);
      if (error_no != 0) {
        DoneCurrentQuery(error_no, ERR_STR);
        current_state_ = IsFatalError(error_no) ? CLOSE_START : CONNECTION_IDLE;
        return true;
      }
      return go_next_state(status, SELECT_DB_WAIT, SELECT_DB_DONE);
    } break;
    case SELECT_DB_WAIT: {
      int error_no = 0;
      int status = ::mysql_select_db_cont(&error_no, &mysql_, in_event);
      if (error_no != 0) {
        DoneCurrentQuery(error_no, ERR_STR);
        current_state_ = IsFatalError(error_no) ? CLOSE_START : CONNECTION_IDLE;
        return true;
      }
      return go_next_state(status, SELECT_DB_WAIT, SELECT_DB_DONE);
    } break;
    case SELECT_DB_DONE: {
      last_selected_db_ = in_process_->DB();
      if (in_process_->QueryContent().empty()) {
        DoneCurrentQuery(0, "db change success");
        current_state_ = CONNECTION_IDLE;
      } else {
        current_state_ = QUERY_START;
      }
      return true;
    } break;
    default:
      CHECK(false);
      break;
  }
  return false;
}

bool MysqlAsyncConnect::HandleStateConnect(int in_event) {

  switch(current_state_) {
    case CONNECT_START: {
      int status = mysql_real_connect_start(&mysql_ret_, &mysql_,
                                            option_.host.c_str(),
                                            option_.user.c_str(),
                                            option_.passwd.c_str(),
                                            option_.dbname.c_str(),
                                            option_.port, NULL, 0);

      return go_next_state(status, CONNECT_WAIT, CONNECT_DONE);
    }break;
    case CONNECT_WAIT: {
      int status = mysql_real_connect_cont(&mysql_ret_, &mysql_, in_event);
      return go_next_state(status, CONNECT_WAIT, CONNECT_DONE);
    }break;
    case CONNECT_DONE: {
      if (!mysql_ret_) {
        ready_ = false;
        current_state_ = CLOSE_START;
        LOG(ERROR) << " connect failed, host:" << option_.host;
        return true;
      }

      ready_ = true;
      current_state_ = CONNECTION_IDLE;
      last_selected_db_ = option_.dbname;

      if (client_) {
        client_->OnConnectReady(this);
      }
      LOG(INFO) << "connect to " << option_.host << "success";
      return true;
    }break;
    case CLOSE_START: {
      int status = mysql_close_start(&mysql_);
      return go_next_state(status, CLOSE_WAIT, CLOSE_DONE);
    }break;
    case CLOSE_WAIT: {
      int status = ::mysql_close_cont(&mysql_, in_event);
      return go_next_state(status, CLOSE_WAIT, CLOSE_DONE);
    } break;
    case CLOSE_DONE: {
      ready_ = false;

      clean_up();
      if (client_) {
        client_->OnConnectionClosed(this);
      }
      current_state_ = CONNECT_DONE;
      return false;
    } break;
    default:
      CHECK(false);
      break;
  }//end switch
  return false;
}

//decide next should connection do
bool MysqlAsyncConnect::HandleStateIDLE(int in_event) {
  if (query_list_.empty()) {
    if (schedule_close_) {
      current_state_ = CLOSE_START;
    }
    return schedule_close_;
  }

  CHECK(!in_process_);

  in_process_ = query_list_.front();
  query_list_.pop_front();

  CHECK(in_process_);

  if (!in_process_->DB().empty() &&
      in_process_->DB() != last_selected_db_) {
    current_state_ = SELECT_DB_START;
    return true;
  }

  current_state_ = QUERY_START;
  return true;
}

//decide can go on or broken
bool MysqlAsyncConnect::IsFatalError(int code) {
  switch(code) {
    case CR_SERVER_LOST:
    case CR_UNKNOWN_ERROR:
    case CR_OUT_OF_MEMORY:
    case CR_SERVER_GONE_ERROR:
      return true;
    default:
      break;
  }
  LOG(ERROR) << "code:" << code << "message:" << ERR_STR;
  return false;
}

void MysqlAsyncConnect::HandleState(int in_event) {

  bool st_continue = true;
  VLOG_IF(2, in_event) << " event:" << MysqlWaitStatusString(in_event);

  while(st_continue) {
    VLOG(1) << "go state:" << current_state_;

    switch(current_state_) {
      case CONNECT_START:
      case CONNECT_WAIT:
      case CONNECT_DONE:
      case CLOSE_START:
      case CLOSE_WAIT:
      case CLOSE_DONE: {
        st_continue = HandleStateConnect(in_event);
      }break;
      case CONNECTION_IDLE: {
        st_continue = HandleStateIDLE(in_event);
      } break;
      case SELECT_DB_START:
      case SELECT_DB_WAIT:
      case SELECT_DB_DONE: {
        CHECK(in_process_);
        st_continue = HandleStateSelectDB(in_event);
      } break;
      case QUERY_START: {
        CHECK(in_process_);

        const std::string& content = in_process_->QueryContent();
        if (content.empty()) {
          DoneCurrentQuery(-9999, "empty query");
          current_state_ = CONNECTION_IDLE;
          st_continue = true;
          break;
        }

        int err_no = 0;
        int status = ::mysql_real_query_start(&err_no,
                                              &mysql_,
                                              content.c_str(),
                                              content.size());
        if (err_no != 0) {
          DoneCurrentQuery(err_no, ERR_STR);
          current_state_ = IsFatalError(err_no) ? CLOSE_START : CONNECTION_IDLE;
          st_continue = true;
          break;
        }

        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      } break;
      case QUERY_WAIT: {
        CHECK(in_process_);

        int err_no = 0;
        int status = ::mysql_real_query_cont(&err_no, &mysql_, in_event);
        if (err_no_ != 0) {
          DoneCurrentQuery(err_no, ERR_STR);
          current_state_ = IsFatalError(err_no) ? CLOSE_START : CONNECTION_IDLE;
          st_continue = true;
          break;
        }
        st_continue = go_next_state(status, QUERY_WAIT, QUERY_RESULT_READY);
      } break;
      case QUERY_RESULT_READY: {
        CHECK(in_process_);

        result_ = ::mysql_use_result(&mysql_);
        if (result_ == NULL) {
          int code = ::mysql_errno(&mysql_);
          DoneCurrentQuery(code, ERR_STR);
          current_state_ = IsFatalError(code) ? CLOSE_START : CONNECTION_IDLE;
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
        CHECK(in_process_ && result_);

        if (!result_row_) {

          int code = ::mysql_errno(&mysql_);
          const char* message = code != 0 ? ERR_STR : NULL;

          //code == 0 mean /* EOF.no more rows */
          in_process_->SetResultRows(::mysql_num_rows(result_));
          ::mysql_free_result(result_);

          DoneCurrentQuery(code, message);

          st_continue = true;
          current_state_ = CONNECTION_IDLE;
          break;
        }

        auto& heards = in_process_->MutableHeaders();
        if (heards.empty()) {
          ParseResultDesc(result_, in_process_.get());
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
        in_process_->PendingRow(std::move(row));

        st_continue = go_next_state(0, FETCH_ROW_START, FETCH_ROW_START);
      } break;
      default: {
        CHECK(false);
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

void MysqlAsyncConnect::DoneCurrentQuery(int code, const char* message) {
  if (!in_process_) return;

  if (code != 0) {
    in_process_->SetCode(code, message != NULL ? message : "");
  }

  in_process_->SetAffectedRows(::mysql_affected_rows(&mysql_));
  in_process_->OnQueryDone();
  in_process_.reset();
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
  //pump->RemoveFdEvent(fd_event_.get());
  if (timeout_->IsAttached()) {
    pump->RemoveTimeoutEvent(timeout_.get());
  }
  fd_event_->DisableAll();
}

void MysqlAsyncConnect::WaitMysqlStatus(int status) {

  base::EventPump* pump = loop_->Pump();
  if (!fd_event_) {
    int fd = ::mysql_get_socket(&mysql_);
    fd_event_ = base::FdEvent::Create(fd, base::LtEv::LT_EVENT_NONE);
    fd_event_->GiveupOwnerFd();
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
  if (current_state_ == CONNECTION_IDLE && !in_process_) {
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

  current_state_ = CONNECT_INIT;
}

void MysqlAsyncConnect::do_connection_check() {

  uint64_t start = base::time_us();
  ready_ = ::mysql_ping(&mysql_) == 0;
  LOG(INFO) << "::mysql_ping spend " << base::time_us()-start << "(us)";

  if (ready_) return;

  schedule_close_ = true;
}

void MysqlAsyncConnect::clean_up() {
  CHECK(loop_->IsInLoopThread());

  auto pump = loop_->Pump();
  if (fd_event_) {
    pump->RemoveFdEvent(fd_event_.get());
    pump->RemoveTimeoutEvent(timeout_.get());
    fd_event_.reset();
  }

  if (in_process_) {
    in_process_->SetCode(-1, "closed");
    in_process_->OnQueryDone();
    in_process_.reset();
  }

  for (auto& query : query_list_) {
    query->SetCode(-1, "closed");
    query->OnQueryDone();
  }
  query_list_.clear();
}

void MysqlAsyncConnect::Connect() {
  current_state_ = CONNECT_START;
  loop_->PostTask(NewClosure(std::bind(&MysqlAsyncConnect::HandleState, this, 0)));
  checker_.Start(60 * 1000, std::bind(&MysqlAsyncConnect::do_connection_check, this));
}

bool MysqlAsyncConnect::SyncConnect() {
  mysql_ret_ = mysql_real_connect(&mysql_,
                                  option_.host.c_str(),
                                  option_.user.c_str(),
                                  option_.passwd.c_str(),
                                  option_.dbname.c_str(),
                                  option_.port, NULL, 0);
  if (mysql_ret_ == NULL) {
    return false;
  }
  ready_ = true;
  current_state_ = CONNECTION_IDLE;
  checker_.Start(60 * 1000, std::bind(&MysqlAsyncConnect::do_connection_check, this));
  return true;
}

void MysqlAsyncConnect::Close() {
  CHECK(loop_->IsInLoopThread());

  schedule_close_ = true;
  if (!in_process_ && current_state_ == QUERY_START) {
    HandleState(0);
  }
}

bool MysqlAsyncConnect::SyncClose() {
  CHECK(loop_->IsInLoopThread());

  clean_up();
  ready_ = false;
  mysql_close(&mysql_);
  current_state_ = CLOSE_DONE;
  return true;
}

}//end lt

