#ifndef REQEUST_HANDLER_H_
#define REQEUST_HANDLER_H_
namespace net {

//runing on the workder thread
class ReqeustHandler {
public:
  ReqeustHandler();
  ~ReqeustHandler();

  virtual HandlerHttpReqeust();
  virtual HandleTcpMessage();
private:

};

}
#endif