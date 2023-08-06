if [  -n "$(uname -a | grep Ubuntu)" ]; then
  echo "debian based system, install system-wide deps...."
  apt install build-essential vim cmake git wget python3 bash-completion
  apt install libunwind-dev zlib1g-dev \
    libssl-dev libev-dev libevent-dev libssl-dev libc-ares-dev libgoogle-perftools-dev
else
  echo "none-debian system plz install system-wide deps manually...."
fi
