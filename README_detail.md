

# profiler

```bash
sudo apt install libgoogle-perftools-dev google-perftools
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so CPUPROFILE=out.prof ./bin/http_benchmark_server
google-pprof --pdf ./bin/http_benchmark_server out.prof > out.pdf
```

### Http2 testing

- H2C
```
./bin/lt_http2_server --v=26
# n: null downloaded data
./nghttp -vn http://localhost:5006/json
./nghttp -vn http://localhost:5006/push

```

- H2 with self signed certification
```
./bin/lt_http2_server --v=26 --ssl --key=../ltio/cert/key.pem --cert=../ltio/cert/cert.pem
curl -k -v --http2-prior-knowledge  "https://localhost:5006/json"
# curl client 不支持server push, 所以这里请求push会在服务端报错
./nghttp -v --key=../../ltio/cert/key.pem --cert=../../ltio/cert/cert.pem https://localhost:5006/json
```
