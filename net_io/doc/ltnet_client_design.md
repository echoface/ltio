# 异步长链接client如何设计

在NIO的设计下, 任何基于task loop模型设计的client都需要面临一些问题.
基于长链接复用的client设计

在ltnet 中,
- Client
  - loop // 一个client 工作的loop, 用于管理具体的client channel
  - connector // client连接器, 用于创建具体的网络socket 
