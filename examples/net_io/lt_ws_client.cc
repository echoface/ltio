#include "base/closure/closure_task.h"
#include "net_io/clients/ws_client.h"
#include "base/message_loop/repeating_timer.h"

using lt::net::WsClient;

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  base::MessageLoop loop("main");
  loop.Start();

  base::RepeatingTimer timer(&loop);

  auto client = WsClient::New(&loop);

  client->WithOpen([&](const WsClient::RefClient& cli) {
    LOG(INFO) << "websocket client opened";
    lt::net::RefWebsocketFrame msg(new lt::net::WebsocketFrame(WS_OP_TEXT));
    msg->AppendData("hello", strlen("hello"));
    cli->Send(msg);

    timer.Start(1000, [cli]() {
      LOG(INFO) << "send pong message";
      lt::net::RefWebsocketFrame pong(new lt::net::WebsocketFrame(WS_OP_PONG));
      cli->Send(pong);
      cli->Send(pong);
      cli->Send(pong);
      cli->Send(pong);
    });
  });

  client->WithClose([](const WsClient::RefClient& cli) {
    LOG(INFO) << "websocket client closed";
  });

  client->WithError([&](const WsClient::RefClient& cli, WsClient::Error err) {
    LOG(INFO) << "websocket client err:" << err;
    LOG(INFO) << "quit loop for error happened";
    loop.QuitLoop();
  });

  client->WithMessage([&](const WsClient::RefClient& cli,
                         const lt::net::RefWebsocketFrame& msg) {

    LOG(INFO) << "websocket got a server message" << msg->Dump();
    if (msg->IsPong()) {
      LOG(INFO) << "websocket got a server pong message" << msg->Dump();
      return;
    }
    loop.PostDelayTask(NewClosure([=]() {
      cli->Send(msg);
    }), 1000);
  });

  client->Dial("ws://127.0.0.1:5007", "/chat");

  loop.WaitLoopEnd();
  LOG(INFO) << "main leave";
}
