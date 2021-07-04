openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -nodes -days 365
#openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout selfsigned.key -out selfsigned.crt

echo "client side, use follow cmd see the ca detail"
echo "openssl s_client -showcerts 127.0.0.1:5006"
echo "curl -v --cacert build/cert/cert.pem https://localhost:5006/ping"

