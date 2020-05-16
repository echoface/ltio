#ifndef LIGHTING_NET_SOCKET_UTILS_H
#define LIGHTING_NET_SOCKET_UTILS_H

#include "base/ip_endpoint.h"
#include <arpa/inet.h>
#include <string>

/* about this code, a beeter refrence is muduo code, most of this from chenshuo's impliment*/
namespace lt {
namespace net {
namespace socketutils {

#define SocketFd int

SocketFd CreateNoneBlockTCP(sa_family_t family, int type = 0);
SocketFd CreateNoneBlockUDP(sa_family_t family, int type = 0);

SocketFd CreateNonBlockingSocket(sa_family_t family);

int ListenSocket(SocketFd fd);

int BindSocketFd(SocketFd fd, const struct sockaddr* addr);

int SocketConnect(SocketFd fd, const struct sockaddr* addr, int* err);

SocketFd AcceptSocket(SocketFd fd, struct sockaddr* addr, int* err);

ssize_t Read(SocketFd fd, void* buf, size_t count);

ssize_t Write(SocketFd fd, const void* buf, size_t count);

ssize_t ReadV(SocketFd fd, const struct iovec* iov, int iovcnt);

void CloseSocket(SocketFd fd);

void ShutdownWrite(SocketFd fd);

int GetSocketError(SocketFd fd);

bool GetPeerEndpoint(int sock, IPEndPoint* ep);
bool GetLocalEndpoint(int sock, IPEndPoint* ep);

struct sockaddr GetPeerAddrIn(int sockfd);
struct sockaddr GetLocalAddrIn(int sockfd);

bool IsSelfConnect(int sockfd);

bool ReUseSocketPort(SocketFd, bool reuse);
bool ReUseSocketAddress(SocketFd socket_fd, bool reuse);

void KeepAlive(SocketFd, bool alive);

}}}
#endif

