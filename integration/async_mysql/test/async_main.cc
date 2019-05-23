#include <iostream>
#include "../mysql_async_con.h"
#include "../mysql_client_impl.h"

using namespace lt;

MysqlOptions option = {
  .host = "10.9.158.21",
  //.host = "127.0.0.1",
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

  auto client = std::shared_ptr<MysqlAsyncClientImpl>(new MysqlAsyncClientImpl(&loop, 1));
  client->InitWithOption(option);

  auto callback = [&](RefQuerySession qs)->void {
    std::ostringstream oss;
    for (const auto&  field : qs->ColumnHeaders()) {
      oss <<  field << " \t| ";
    }
    for (const auto& row : qs->Result()) {
      oss << "\n";
      for (auto& v : row) {
        oss << v << "\t| ";
      }
    }
    LOG(INFO) << "query:" << qs->QueryContent()
      << "\ncode:" << qs->Code()
      << "\nmessage:" << qs->ErrorMessage()
      << "\nresultcount:" << qs->ResultRows()
      << "\naffectedline:" << qs->AffectedRows()
      << "\nresult:\n" << oss.str();
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
    } else {
      client->Close();
      client.reset();

      loop.QuitLoop();
    }
  }

  loop.WaitLoopEnd();

  mysql_library_end();
  return 0;
}

