/**
 * @file morgusock.h
 * @brief defines a multiplatform interface to interact with sockets
 */
#pragma once

#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif // _MSC_VER

typedef SOCKET MorguSocket;
#define MORGUSOCK_INVALID INVALID_SOCKET
#define MORGUSOCK_CLOSE(s) closesocket(s)
#define MORGUSOCK_MAX_IO (1 << 30)
#define MORGUSOCK_POLL(fds, n, ms) WSAPoll((fds), (ULONG)(n), (ms))

#else // _WIN32

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

typedef int MorguSocket;
#define MORGUSOCK_INVALID (-1)
#define MORGUSOCK_CLOSE(s) close(s)
#define MORGUSOCK_POLL(fds, n, ms) poll((fds), (nfds_t)(n), (ms))
#endif // _WIN32

#ifdef _MSC_VER
#define MORGUSOCK_API static __inline
#else
#define MORGUSOCK_API static inline
#endif // _MSC_VER

/**
 * @brief initializes socket interface
 * @note call once at the beginning of the program
 *
 * @return 0 on success
 */
MORGUSOCK_API int MorguSockInit(void) {
#ifdef _WIN32
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2, 2), &wsa);
#else
  signal(SIGPIPE, SIG_IGN); /* writing to a closed socket returns EPIPE instead of killing us */
  return 0;
#endif
}

/**
 * @brief Frees network resources used by socket interface
 * @note Call once on program end
 */
MORGUSOCK_API void MorguSockClean(void) {
#ifdef _WIN32
  WSACleanup();
#endif
}

/**
 * @brief Gets last error issued by the socket interface
 * 
 * @return Socket error code (see @ref MorguSockStringError() for readable errors)
 */
MORGUSOCK_API int MorguSockErrNo(void) {
#ifdef _WIN32
  return WSAGetLastError();
#else
  return errno;
#endif
}

/**
 * @brief Transforms socket interface error code into human readable message
 * 
 * @param err Socket error code
 * @param buf Buffer pointer
 * @param buflen Buffer length
 * @return Same buffer ( @p buf ) 
 */
MORGUSOCK_API const char* MorguSockStringError(int err, char* buf, size_t buflen) {
#ifdef _WIN32
  DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                           (DWORD)err, 0, buf, (DWORD)buflen, NULL);
  if (n == 0)
    snprintf(buf, buflen, "error %d", err);
  return buf;
#else
  snprintf(buf, buflen, "%s", strerror(err));
  return buf;
#endif
}

/**
 * @brief Checks whether the last error code matches @c EWOULDBLOCK
 * 
 * @return 1 if true, 0 if false
 */
MORGUSOCK_API int MorguSockWouldBlock(void) {
#ifdef _WIN32
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

/**
 * @brief Releases resources associated with the socket
 * 
 * @param s Socket to be released
 */
MORGUSOCK_API void MorguSockClose(MorguSocket s) {
  if (s != MORGUSOCK_INVALID)
    MORGUSOCK_CLOSE(s);
}

/**
 * @brief Toggles nonblocking mode for sockets
 * 
 * If socket is set to nonblocking mode, receive and send functions
 * will return immediately (setting @ref MorguSockWouldBlock() error )
 * 
 * @param s Target socket
 * @param enable Toggle indicator
 * @return 0 if successful
 */
MORGUSOCK_API int MorguSockSetNonblock(MorguSocket s, int enable) {
#ifdef _WIN32
  u_long mode = enable ? 1 : 0;
  return ioctlsocket(s, (long int)FIONBIO, &mode);
#else
  int flags = fcntl(s, F_GETFL, 0);
  if (flags < 0)
    return -1;
  flags = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return fcntl(s, F_SETFL, flags);
#endif
}

/**
 * @brief Enables "reuse address" option, allowing the socket to bind to a recently used address
 * 
 * @param s Target socket
 * @return 0 if successful
 */
MORGUSOCK_API int MorguSockSetReuseAddr(MorguSocket s) {
  int on = 1;
  return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof on);
}

/**
 * @brief Disables Nagle's algorithm (instanteneous data forwarding instead of grouping with latency)
 * 
 * @param s Target socket
 * @return 0 if successful
 */
MORGUSOCK_API int MorguSockSetNoDelay(MorguSocket s) {
  int on = 1;
  return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof on);
}

/**
 * @brief Sets socket's receive and send timeouts
 * 
 * @param s Target socket
 * @param ms timeout in milliseconds (0 for no timeout)
 * @return 0 if successful
 */
MORGUSOCK_API int MorguSockSetTimeout(MorguSocket s, int ms) {
#ifdef _WIN32
  DWORD t = (DWORD)ms;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t) != 0)
    return -1;
  return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&t, sizeof t);
#else
  struct timeval tv;
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) != 0)
    return -1;
  return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
#endif
}

/**
 * @brief Creates a socket that is connected to host:port
 * 
 * @param host Remote host
 * @param port Remote port
 * @return Socket representing client or @ref MORGUSOCK_INVALID if connection failed 
 */
MORGUSOCK_API MorguSocket MorguSockConnectTCP(const char* host, const char* port) {
  struct addrinfo hints, *res = NULL, *p;
  MorguSocket s = MORGUSOCK_INVALID;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port, &hints, &res) != 0)
    return MORGUSOCK_INVALID;

  for (p = res; p != NULL; p = p->ai_next) {
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s == MORGUSOCK_INVALID)
      continue;
    if (connect(s, p->ai_addr, (int)p->ai_addrlen) == 0)
      break;
    MORGUSOCK_CLOSE(s);
    s = MORGUSOCK_INVALID;
  }
  freeaddrinfo(res);
  return s;
}

/**
 * @brief Creates a socket that listens for incoming connections
 * 
 * @param port local port
 * @param backlog max amount of pending connections
 * @return Listener socket or @ref MORGUSOCK_INVALID on failure 
 */
MORGUSOCK_API MorguSocket MorguSockListenTCP(const char* port, int backlog) {
  struct addrinfo hints, *res = NULL, *p;
  MorguSocket s = MORGUSOCK_INVALID;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, port, &hints, &res) != 0)
    return MORGUSOCK_INVALID;

  for (p = res; p != NULL; p = p->ai_next) {
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s == MORGUSOCK_INVALID)
      continue;

    MorguSockSetReuseAddr(s);
    if (p->ai_family == AF_INET6) {
      int off = 0; /* accept IPv4-mapped connections too */
      setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&off, sizeof off);
    }
    if (bind(s, p->ai_addr, (int)p->ai_addrlen) == 0 && listen(s, backlog) == 0)
      break;

    MORGUSOCK_CLOSE(s);
    s = MORGUSOCK_INVALID;
  }
  freeaddrinfo(res);
  return s;
}

/**
 * @brief Accept an incoming connection on provided socket
 * 
 * @param listener Listener socket (created by @ref MorguSockListenTCP() )
 * @return A socket representing a client or @ref MORGUSOCK_INVALID on failure
 */
MORGUSOCK_API MorguSocket MorguSockAccept(MorguSocket listener) {
  return accept(listener, NULL, NULL);
}

/**
 * @brief Accept an incoming connection and report the peer's address
 * 
 * @param listener Target (listener) socket
 * @param host Out parameter to write peer's address to
 * @param hostlen host's string buffer length
 * @return A socket representing a client or @ref MORGUSOCK_INVALID on failure
 */
MORGUSOCK_API MorguSocket MorguSockAcceptFrom(MorguSocket listener, char* host, size_t hostlen) {
  struct sockaddr_storage ss;
#ifdef _WIN32
  int len = (int)sizeof ss;
#else
  socklen_t len = sizeof ss;
#endif
  MorguSocket c = accept(listener, (struct sockaddr*)&ss, &len);
  if (c != MORGUSOCK_INVALID && host && hostlen) {
    if (getnameinfo((struct sockaddr*)&ss, len, host,
                    (
#ifdef _WIN32
                        DWORD
#else
                        socklen_t
#endif
                        )hostlen,
                    NULL, 0, NI_NUMERICHOST) != 0) {
      host[0] = '\0';
    }
  }
  return c;
}

/**
 * @brief Sends bytes over the socket connection
 * 
 * @param s Target socket
 * @param buf Data array to send
 * @param len Length of data array
 * @return How many bytes were sent or -1 on failure 
 */
MORGUSOCK_API int MorguSockSend(MorguSocket s, const void* buf, size_t len) {
#ifdef _WIN32
  return send(s, (const char*)buf, (int)len, 0);
#else
  return (int)send(s, buf, len, 0);
#endif
}

/**
 * @brief Receive up to @p len bytes
 * 
 * @param s Target socket
 * @param buf Byte buffer
 * @param len Max amount of bytes to receive
 * @return 0 if peer closed, -1 on error, amount of bytes read otherwise
 */
MORGUSOCK_API int MorguSockReceive(MorguSocket s, void* buf, size_t len) {
#ifdef _WIN32
  return recv(s, (char*)buf, (int)len, 0);
#else
  return (int)recv(s, buf, len, 0);
#endif
}

/**
 * @brief Send exactly @p len bytes
 * @note Loops on partial writes
 * @param s Target socket
 * @param buf Data buffer
 * @param len Buffer length
 * @return 0 on success
 */
MORGUSOCK_API int MorguSockSendAll(MorguSocket s, const void* buf, size_t len) {

  size_t chunk = len > MORGUSOCK_MAX_IO ? MORGUSOCK_MAX_IO : len;
  const char* p = (const char*)buf;
  while (len > 0) {
    int n = MorguSockSend(s, p, chunk);
    if (n <= 0)
      return -1;
    p += n;
    len -= (size_t)n;
  }
  return 0;
}

/**
 * @brief Receive exactly @p len bytes
 * @param s Target socket
 * @param buf Data buffer
 * @param len How many bytes to receive
 * @return 0 on success, 1 if peer closed early, -1 on error
 */
MORGUSOCK_API int MorguSockReceiveAll(MorguSocket s, void* buf, size_t len) {
  char* p = (char*)buf;
  size_t chunk = len > MORGUSOCK_MAX_IO ? MORGUSOCK_MAX_IO : len;
  while (len > 0) {
    int n = MorguSockReceive(s, p, chunk);
    if (n < 0)
      return -1;
    if (n == 0)
      return 1;
    p += n;
    len -= (size_t)n;
  }
  return 0;
}

 /**
  * @brief Poll until the socket is readable
  * 
  * @param s Target socket
  * @param timeout_ms Max time to wait in milliseconds (< 0 waits forever)
  * @return 1 if readable, 0 on timeout, -1 on error
  */
MORGUSOCK_API int MorguSockWaitReadable(MorguSocket s, int timeout_ms) {
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
  if (r < 0)
    return -1;
  return r > 0 ? 1 : 0;
}

#undef MORGUSOCK_API
#undef MORGUSOCK_POLL
#undef MORGUSOCK_CLOSE