#include "mysql_async_con.h"
#include <iostream>


MysqlOptions option = {
  .host = "10.9.158.21",
  .port = 3306,
  .user = "fancy_test",
  .passwd = "fancy_test",
  .dbname = "mysql",
  .query_timeout = 5000,
};

int main(int argc, char** argv) {
  mysql_library_init(0, NULL, NULL);

  if (argc > 1 && std::string(argv[1]) == "localhost") {
    option.host = "localhost";
    option.user = "root";
    option.passwd = "";
  }

  base::MessageLoop loop;

  loop.SetLoopName("main");
  loop.Start();

  MysqlConnection* g_conn = new MysqlConnection(NULL, &loop);

  loop.PostTask(NewClosure([&]()->void {
    g_conn->InitConnection(option);
  }));

  loop.PostDelayTask(NewClosure([&]() {
    LOG(INFO) << "start query mysql db";
  }), 3000);

  std::string content;
  while(content != "exit") {
    std::getline(std::cin, content);
    LOG(INFO) << "got query content:" << content;
    if (content != "exit") {
      loop.PostTask(NewClosure([=]()->void {
      }));
    }
  }

  loop.WaitLoopEnd();

  delete g_conn;
  mysql_library_end();
  return 0;
}

