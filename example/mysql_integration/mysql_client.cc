#include <mysql/mysql.h>
#include <iostream>
#include "base/message_loop/message_loop.h"

/* Maintaining a list of queries to run. */
struct query_entry {
  struct query_entry *next;
  char *query;
  int index;
};

enum State{
  CONNECT_START,
  CONNECT_WAITING,
  CONNECT_DONE,

  QUERY_START,
  QUERY_WAITING,
  QUERY_RESULT_READY,

  FETCH_ROW_START,
  FETCH_ROW_WAITING,
  FETCH_ROW_RESULT_READY,

  CLOSE_START,
  CLOSE_WAITING,
  CLOSE_DONE
};


static struct query_entry *query_list;
static struct query_entry **tail_ptr= &query_list;
static int query_counter= 0;

struct MysqlConnection {
  int current_state;                   // State machine current state

  MYSQL mysql;
  MYSQL *ret;
  MYSQL_ROW row;
  MYSQL_RES *result;
  struct query_entry *current_query_entry;
  int err;
  base::RefFdEvent fd_event;
  base::TimeoutEvent* to_event;

  void OnError();
  void OnClose();
  void OnReadable();
  void OnWritable();

  void OnTimeOut();
};

void UpdateEvent(MysqlConnection* conn, int status);

void MysqlConnection::OnTimeOut() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  switch(current_state) {
    case CONNECT_WAITING:{
      int status = ::mysql_real_connect_cont(&ret, &mysql, MYSQL_WAIT_WRITE);
      std::cout << __FUNCTION__ << " mysql_real_connect_cont return:" << status << std::endl;
      if (status) {
        UpdateEvent(this, status);
      } else {
        current_state = QUERY_START;
        fd_event->DisableAll();
      }
    }break;
    default:
    break;
  }
}

void MysqlConnection::OnClose() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
}

void MysqlConnection::OnError() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
}

void MysqlConnection::OnReadable() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  switch(current_state) {
    case CONNECT_WAITING:{
      int status = ::mysql_real_connect_cont(&ret, &mysql, MYSQL_WAIT_WRITE);
      std::cout << __FUNCTION__ << " mysql_real_connect_cont return:" << status << std::endl;
      if (status) {
        UpdateEvent(this, status);
      } else {
        current_state = QUERY_START;
        fd_event->DisableAll();
      }
    }break;
    default:
    break;
  }
}

void MysqlConnection::OnWritable() {
  std::cout << __FUNCTION__ << " reached" << std::endl;
  switch(current_state) {
    case CONNECT_WAITING:{
      int status = ::mysql_real_connect_cont(&ret, &mysql, MYSQL_WAIT_WRITE);
      std::cout << __FUNCTION__ << " mysql_real_connect_cont return:" << status << std::endl;
      if (status) {
        UpdateEvent(this, status);
      } else {
        current_state = QUERY_START;
        fd_event->DisableAll();
      }
    }break;
    default:
    break;
  }
}


MysqlConnection* g_conn = NULL;
base::MessageLoop loop;

void InitMysqlConnection();

void UpdateEvent(MysqlConnection* conn, int status) {
  std::cout << __FUNCTION__ << " status:" << status << std::endl;

  if (!conn->fd_event) {
    int fd = mysql_get_socket(&conn->mysql);
    conn->fd_event = base::FdEvent::Create(fd, base::LtEv::LT_EVENT_NONE);

    conn->fd_event->SetErrorCallback(std::bind(&MysqlConnection::OnError, g_conn));
    conn->fd_event->SetCloseCallback(std::bind(&MysqlConnection::OnClose, g_conn));
    conn->fd_event->SetReadCallback(std::bind(&MysqlConnection::OnReadable, g_conn));
    conn->fd_event->SetWriteCallback(std::bind(&MysqlConnection::OnWritable, g_conn));

    base::EventPump* pump = base::MessageLoop::Current()->Pump();
    pump->InstallFdEvent(conn->fd_event.get());
  }

  //base::LtEv wait_event = base::LtEv::LT_EVENT_NONE;
  if (status & MYSQL_WAIT_READ) {
    std::cout << __FUNCTION__ << " wait for reading" << std::endl;
    conn->fd_event->EnableReading();
  } else {
    std::cout << __FUNCTION__ << " disable reading" << std::endl;
    conn->fd_event->DisableReading();
  }

  if (status & MYSQL_WAIT_WRITE) {
    std::cout << __FUNCTION__ << " wait for writing" << std::endl;
    conn->fd_event->EnableWriting();
  } else {
    std::cout << __FUNCTION__ << " disable writing" << std::endl;
    conn->fd_event->DisableWriting();
  }


  if (status & MYSQL_WAIT_TIMEOUT) {
    if (conn->to_event->IsAttached()) {
      base::MessageLoop::Current()->Pump()->RemoveTimeoutEvent(conn->to_event);
    }

    int timeout = 1000 * mysql_get_timeout_value(&conn->mysql);
    conn->to_event->UpdateInterval(timeout);

    std::cout << __FUNCTION__ << " add timeout wait:" << timeout << std::endl;
    base::MessageLoop::Current()->Pump()->AddTimeoutEvent(conn->to_event);
  }
/*
   int status = 0;
   if (pfd.revents & POLLIN) status |= MYSQL_WAIT_READ;
   if (pfd.revents & POLLOUT) status |= MYSQL_WAIT_WRITE;
   if (pfd.revents & POLLPRI) status |= MYSQL_WAIT_EXCEPT;
   return status;
*/
}

void Connect(MysqlConnection* conn) {

  int status = ::mysql_real_connect_start(&conn->ret,
                                          &conn->mysql,
                                          "127.0.0.1",
                                          "root",
                                          "",
                                          "test",
                                          0,
                                          NULL,
                                          0);
  if (status) {
    UpdateEvent(conn, status);
    conn->current_state = CONNECT_WAITING;
  } else {
    std::cout << __FUNCTION__ << " connect mysql success" << std::endl;
    conn->current_state = QUERY_START;
  }
}

void ContentMain() {

  InitMysqlConnection();

  Connect(g_conn);
};


int main(int argc, char** argv) {

  int err = mysql_library_init(0, NULL, NULL);
  if (err) {
    fprintf(stderr, "Fatal: mysql_library_init() returns error: %d\n", err);
    exit(1);
  }

  loop.SetLoopName("main");
  loop.Start();

  loop.PostTask(NewClosure(std::bind(ContentMain)));
  loop.WaitLoopEnd();
  return 0;
}

void InitMysqlConnection() {

  g_conn = new MysqlConnection;
  ::mysql_init(&g_conn->mysql);
  ::mysql_options(&g_conn->mysql, MYSQL_OPT_NONBLOCK, NULL);
  ::mysql_options(&g_conn->mysql, MYSQL_READ_DEFAULT_GROUP, "async_queries");

  uint32_t default_timeout = 5; //second
  ::mysql_options(&g_conn->mysql, MYSQL_OPT_READ_TIMEOUT, &default_timeout);
  ::mysql_options(&g_conn->mysql, MYSQL_OPT_WRITE_TIMEOUT, &default_timeout);
  ::mysql_options(&g_conn->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &default_timeout);

  g_conn->to_event = new base::TimeoutEvent(default_timeout, false);
  g_conn->to_event->InstallTimerHandler(NewClosure(std::bind(&MysqlConnection::OnTimeOut, g_conn)));
}

