#include <thirdparty/catch/catch.hpp>

#include "base/coroutine/co_runner.h"
#include "base/message_loop/message_loop.h"
#include "net_io/co_so/io_service.h"

TEST_CASE("co_so.ioservice", "[co_so ioservice run]") {
  FLAGS_v = 26;

  base::MessageLoop loop("io");
  loop.Start();

  co_go &loop << [&]() {

    IPEndPoint address("127.0.0.1", 5006);
    coso::IOService ioservice(address);
    ioservice.WithProtocal("tcp");
    ioservice.Run();
  };
  loop.WaitLoopEnd();
}
