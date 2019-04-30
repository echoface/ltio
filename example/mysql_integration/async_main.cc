#include <iostream>
#include "mysql_async_con.h"
#include "mysql_client_impl.h"

MysqlOptions option = {
  .host = "localhost",
  .port = 3306,
  .user = "root",
  .passwd = "",
  .dbname = "mysql",
  .query_timeout = 5000,
};

int main(int argc, char** argv) {
  mysql_library_init(0, NULL, NULL);

  base::MessageLoop loop;
  loop.SetLoopName("main");
  loop.Start();

  auto client = new MysqlAsyncClientImpl(&loop, 1);
  client->InitWithOption(option);

  auto callback = [&](RefQuerySession qs)->void {
    std::ostringstream oss;
    for (const auto& row : qs->Result()) {
      for (const auto& field : row) {
        oss << field << "\t|\t";
      }
      oss << "\n";
    }
    LOG(INFO) << "query back and result is:" << qs->Code();
    LOG(INFO) << oss.str();
  };

  std::string content;
  while(content != "exit") {
    std::getline(std::cin, content);
    LOG(INFO) << "got query content:" << content;

    if (content != "exit") {
      RefQuerySession qs = QuerySession::New();

      auto onback = std::bind(callback, qs);
      qs->UseDB("mysql").Query(content).Then(onback);
      client->PendingQuery(qs);
    }
  }

  loop.WaitLoopEnd();

  mysql_library_end();
  return 0;
}

