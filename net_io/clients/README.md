#Client Desc Design Description


```
ClientRouter 里面管理和维护一组client, 当发起client请求时通过
router拿到一个使用不同方式路由到的Client对象发起请求.

ClientRouter: eg: 一组Redis服务
  - Client 1: scheme:host:port
    **own a connector, 负责异步完成连接**
    - ClientChannel.1 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.2 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.N : (ProtocolService-SocketChannel) = 1:1

  - Client 2: scheme:host:port
    **own a connector, 负责异步完成连接**
    - ClientChannel.1 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.2 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.N : (ProtocolService-SocketChannel) = 1:1

  - Client 3: scheme:host:port
    **own a connector, 负责异步完成连接**
    - ClientChannel.1 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.2 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.N : (ProtocolService-SocketChannel) = 1:1

  - Client 4: scheme:host:port
    **own a connector, 负责异步完成连接**
    - ClientChannel.1 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.2 : (ProtocolService-SocketChannel) = 1:1
    - ClientChannel.N : (ProtocolService-SocketChannel) = 1:1
```
