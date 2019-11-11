# 异步长链接client如何设计

在NIO的设计下, 任何基于task loop模型设计的client都需要面临一些问题.
- 长链接复用client设计如何优雅的建立与管理链接
- 如何理想的、干净的断开链接

在ltnet 中,
## client
- Client
  - loop // 一个client 工作的loop, 用于管理具体的client channel
  - connector // client连接器, 用于创建具体的网络socket
  - ClientChannels // 一组客户端链接的, 链接同一个远端节点

## router
router是一个客户端链接管理器, 其中管理着一组或者多组连接到远端的client, 他们负责各种client使用的
策略,比如说轮询调度(roundrobin), 哈希调度(hash), 一致性hash调度等等, 目前ltnet.client中大致实现了
如下集中简单的router方式
- roundrobin

- ringhash rounter
  - a classic ring hash with vnode supported

- meglev router
  - a google implemented software load balance hash method
  - ref: https://static.googleusercontent.com/media/research.google.com/en/pubs/archive/44824.pdf

当client 配置了heartbeat 且 对应的protocolservice支持心跳, 客户端会在client channel端启动一个心跳保持逻辑
- 客户端
  - 通过protocolservice.NewHeartbeat创建对应协议的心跳请求消息
  - 通过具体类型(queue/async)的clientchannel发送消息请求

服务端心跳确认存在两种处理模式, 这个应该有设计者自己决定这种协议的心跳由协议层响应还是由应用层响应
  - 协议层处理
    - decode reponse消息成功后判断是否是heartbeat消息(IsHeartbeat)
    - 若是heartbeat 调用NewHeartbeat创建对应的心跳response并回应

  - 应用层处理
    - decode出来的消息直接返回给具体的服务处理函数处理

  所以对应的protocol service通过自己协议的需求定义这部分处理逻辑即可

