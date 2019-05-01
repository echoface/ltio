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

  google::InitGoogleLogging(argv[0]);
  FLAGS_stderrthreshold=google::INFO;
  FLAGS_colorlogtostderr=true;

  base::MessageLoop loop;
  loop.SetLoopName("main");
  loop.Start();

  auto client = new MysqlAsyncClientImpl(&loop, 1);
  client->InitWithOption(option);

  auto callback = [&](RefQuerySession qs)->void {
    LOG(INFO) << "query:" << qs->QueryContent()
      << "\ncode:" << qs->Code()
      << "\nmessage:" << qs->ErrorMessage()
      << "\nresultcount:" << qs->ResultRows()
      << "\naffectedline:" << qs->AffectedRows();
  };


  std::string content;
#if 1
  std::cout << "press any charactor key start" << std::endl;
  std::cin >> content;
  std::vector<std::string> querys = {
    "show tables",
    "desc db",
    "bacd",
    "desc db",
    "show tables",
  };
  for (auto& query : querys) {
      RefQuerySession qs = QuerySession::New();
      auto onback = std::bind(callback, qs);
      qs->UseDB("mysql").Query(query).Then(onback);
      client->PendingQuery(qs);
      sleep(1);
  }
#else
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
#endif

  loop.WaitLoopEnd();

  mysql_library_end();
  return 0;
}

