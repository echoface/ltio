# README


在原来自己实现Address对象无法同时兼容IPV4/6, 参考chromium项目中对IP/Address/Endpoint的描述, 抽离了
IpAddress/IpEndpoint实现. 并移除了Posix之外的其他依赖和实现. 后期如果不出现异常情况, 不会合并Upstream
的改动;

