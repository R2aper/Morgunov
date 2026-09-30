#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "ws2_32.lib")
  #endif
 
  typedef SOCKET MorguSocket;
  #define MORGUSOCK_INVALID INVALID_SOCKET
  #define MORGUSOCK_CLOSE(s) closesocket(s)
  #define MORGUSOCK_POLL(fds, n, ms) WSAPoll((fds), (ULONG)(n), (ms))
#else
  #include <errno.h>
  #include <fcntl.h>
  #include <poll.h>
  #include <signal.h>
  #include <string.h>
  #include <unistd.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <sys/types.h>
 
  typedef int MorguSocket;
  #define MORGUSOCK_INVALID (-1)
  #define MORGUSOCK_CLOSE(s) close(s)
  #define MORGUSOCK_POLL(fds, n, ms) poll((fds), (nfds_t)(n), (ms))
#endif
 
#ifdef _MSC_VER
  #define MORGUSOCK_API static __inline
#else
  #define MORGUSOCK_API static inline
#endif

/* Call once at program start. Returns 0 on success. */
MORGUSOCK_API int MorguSockInit(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    signal(SIGPIPE, SIG_IGN); /* writing to a closed socket returns EPIPE instead of killing us */
    return 0;
#endif
}

/* Call once at program end. */
MORGUSOCK_API void MorguSockClean(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/* Last socket error code for the calling thread. */
MORGUSOCK_API int MorguSockErrNo(void)
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}
 
/* Human-readable message for an error code, written into buf. */
MORGUSOCK_API const char* MorguSockStringError(int err, char *buf, size_t buflen)
{
#ifdef _WIN32
    DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, (DWORD)err, 0, buf, (DWORD)buflen, NULL);
    if (n == 0) snprintf(buf, buflen, "error %d", err);
    return buf;
#else
    snprintf(buf, buflen, "%s", strerror(err));
    return buf;
#endif
}

/* True if the last error means "would block / try again". */
MORGUSOCK_API int MorguSockWouldBlock(void)
{
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

MORGUSOCK_API void MorguSockClose(MorguSocket s)
{
    if (s != MORGUSOCK_INVALID) MORGUSOCK_CLOSE(s);
}

/* Enable/disable non-blocking mode. Returns 0 on success. */
MORGUSOCK_API int MorguSockSetNonblock(MorguSocket s, int enable)
{
#ifdef _WIN32
    u_long mode = enable ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return -1;
    flags = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(s, F_SETFL, flags);
#endif
}

MORGUSOCK_API int MorguSockSetReuseAddr(MorguSocket s)
{
    int on = 1;
    return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof on);
}

MORGUSOCK_API int MorguSockSetNoDelay(MorguSocket s)
{
    int on = 1;
    return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof on);
}

/* Set send + receive timeouts in milliseconds (0 = no timeout). */
MORGUSOCK_API int MorguSockSetTimeout(MorguSocket s, int ms)
{
#ifdef _WIN32
    DWORD t = (DWORD)ms;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&t, sizeof t) != 0) return -1;
    return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&t, sizeof t);
#else
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) != 0) return -1;
    return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
#endif
}

/* Connect to host:port (port may be a number or service name like "http").
 * Supports IPv4 and IPv6. Returns MORGUSOCK_INVALID on failure. */
MORGUSOCK_API MorguSocket MorguSockConnectTCP(const char *host, const char *port)
{
    struct addrinfo hints, *res = NULL, *p;
    MorguSocket s = MORGUSOCK_INVALID;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
 
    if (getaddrinfo(host, port, &hints, &res) != 0) return MORGUSOCK_INVALID;
 
    for (p = res; p != NULL; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == MORGUSOCK_INVALID) continue;
        if (connect(s, p->ai_addr, (int)p->ai_addrlen) == 0) break;
        MORGUSOCK_CLOSE(s);
        s = MORGUSOCK_INVALID;
    }
    freeaddrinfo(res);
    return s;
}

/* Create a listening TCP socket bound to all interfaces on the given port.
 * Uses dual-stack (IPv4 + IPv6) when available. */
MORGUSOCK_API MorguSocket MorguSockListenTCP(const char *port, int backlog)
{
    struct addrinfo hints, *res = NULL, *p;
    MorguSocket s = MORGUSOCK_INVALID;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
 
    if (getaddrinfo(NULL, port, &hints, &res) != 0) return MORGUSOCK_INVALID;
 
    for (p = res; p != NULL; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == MORGUSOCK_INVALID) continue;
 
        MorguSockSetReuseAddr(s);
        if (p->ai_family == AF_INET6) {
            int off = 0; /* accept IPv4-mapped connections too */
            setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&off, sizeof off);
        }
        if (bind(s, p->ai_addr, (int)p->ai_addrlen) == 0 && listen(s, backlog) == 0) break;
 
        MORGUSOCK_CLOSE(s);
        s = MORGUSOCK_INVALID;
    }
    freeaddrinfo(res);
    return s;
}

/* Accept a connection. Returns MORGUSOCK_INVALID on failure. */
MORGUSOCK_API MorguSocket MorguSockAccept(MorguSocket listener)
{
    return accept(listener, NULL, NULL);
}

/* Accept and also report the peer's address as a "host" string (numeric). */
MORGUSOCK_API MorguSocket MorguSockAcceptFrom(MorguSocket listener, char *host, size_t hostlen)
{
    struct sockaddr_storage ss;
#ifdef _WIN32
    int len = (int)sizeof ss;
#else
    socklen_t len = sizeof ss;
#endif
MorguSocket c = accept(listener, (struct sockaddr *)&ss, &len);
    if (c != MORGUSOCK_INVALID && host && hostlen) {
        if (getnameinfo((struct sockaddr *)&ss, len, host, (
#ifdef _WIN32
            DWORD
#else
            socklen_t
#endif
            )hostlen, NULL, 0, NI_NUMERICHOST) != 0) {
            host[0] = '\0';
        }
    }
    return c;
}

/* Send up to len bytes. Returns bytes sent, or -1 on error. */
MORGUSOCK_API int MorguSockSend(MorguSocket s, const void *buf, size_t len)
{
#ifdef _WIN32
    return send(s, (const char *)buf, (int)len, 0);
#else
    return (int)send(s, buf, len, 0);
#endif
}

/* Receive up to len bytes. Returns bytes read, 0 if peer closed, -1 on error. */
MORGUSOCK_API int MorguSockReceive(MorguSocket s, void *buf, size_t len)
{
#ifdef _WIN32
    return recv(s, (char *)buf, (int)len, 0);
#else
    return (int)recv(s, buf, len, 0);
#endif
}

/* Send exactly len bytes (loops on partial writes). Returns 0 on success, -1 on error. */
MORGUSOCK_API int MorguSockSendAll(MorguSocket s, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    while (len > 0) {
        int n = MorguSockSend(s, p, len);
        if (n < 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

/* Receive exactly len bytes. Returns 0 on success, 1 if peer closed early, -1 on error. */
MORGUSOCK_API int MorguSockReceiveAll(MorguSocket s, void *buf, size_t len)
{
    char *p = (char *)buf;
    while (len > 0) {
        int n = MorguSockReceive(s, p, len);
        if (n < 0) return -1;
        if (n == 0) return 1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

/* Wait until the socket is readable. timeout_ms < 0 waits forever.
 * Returns 1 if readable, 0 on timeout, -1 on error. */
MORGUSOCK_API int MorguSockWaitReadable(MorguSocket s, int timeout_ms)
{
#ifdef _WIN32
    WSAPOLLFD pfd;
#else
    struct pollfd pfd;
#endif
    int r;
    pfd.fd = s;
    pfd.events = POLLIN;
    pfd.revents = 0;
    r = MORGUSOCK_POLL(&pfd, 1, timeout_ms);
    if (r < 0) return -1;
    return r > 0 ? 1 : 0;
}

#undef MORGUSOCK_API