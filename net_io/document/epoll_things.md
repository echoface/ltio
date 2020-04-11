# epoll thing need notice

For these kind of questions, use the source! Among other interesting comments, there is this text:
EPOLLHUP is UNMASKABLE event (...). It means that after we received EOF, poll always returns immediately, making impossible poll() on write() in state CLOSE_WAIT. One solution is evident --- to set EPOLLHUP if and only if shutdown has been made in both directions.
And then the only code that sets EPOLLHUP:

```c++
if (sk->sk_shutdown == SHUTDOWN_MASK || state == TCP_CLOSE)
    mask |= EPOLLHUP;
```

Being `SHUTDOWN_MASK` equal to `RCV_SHUTDOWN |SEND_SHUTDOWN`.

TL; DR; You are right, this flag is only sent when the shutdown has been both for read and write (I reckon that the peer shutdowning the write equals to my shutdowning the read). Or when the connection is closed, of course.

UPDATE: From reading the source code with more detail, these are my conclusions.

About shutdown:

- Doing `shutdown(SHUT_WR)` sends a FIN and marks the socket with `SEND_SHUTDOWN`.
- Doing `shutdown(SHUT_RD)` sends nothing and marks the socket with `RCV_SHUTDOWN`.
- Receiving a FIN marks the socket with `RCV_SHUTDOWN`.

And about epoll:

If the socket is marked with `SEND_SHUTDOWN` and `RCV_SHUTDOWN`, poll will return `EPOLLHUP`.
If the socket is marked with `RCV_SHUTDOWN`, poll will return `EPOLLRDHUP`.
So the HUP events can be read as:

EPOLLRDHUP: you have received FIN or you have called `shutdown(SHUT_RD)`. In any case your reading half-socket is hung, that is, you will read no more data.
EPOLLHUP: you have both half-sockets hung. The reading half-socket is just like the previous point, For the sending half-socket you did something like shutdown(SHUT_WR).

To complete a a graceful shutdown I would do:

Do `shutdown(SHUT_WR)` to send a FIN and mark the end of sending data.
Wait for the peer to do the same by polling until you get a `EPOLLRDHUP`.
Now you can close the socket with grace.
