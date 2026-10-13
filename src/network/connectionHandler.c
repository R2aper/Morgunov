#include "connectionHandler.h"

#include <stdbool.h>
#include <stdlib.h>

#include "morgusock.h"

#define MORGUNOV_MAX_CONNECTIONS (FD_SETSIZE - 1)

typedef struct Client {
  MorguSocket Socket;
  char Address[64];
} Client;

static Client _clients[MORGUNOV_MAX_CONNECTIONS];

static void _ClientsInit(void) {
  for (int i = 0; i < MORGUNOV_MAX_CONNECTIONS; i++) {
    _clients[i].Socket = MORGUSOCK_INVALID;
    _clients[i].Address[0] = '\0';
  }
}

static int _ClientsAdd(MorguSocket s) {
  for (int i = 0; i < MORGUNOV_MAX_CONNECTIONS; i++) {
    if (_clients[i].Socket == MORGUSOCK_INVALID) {
      _clients[i].Socket = s;
      return i;
    }
  }
  return -1;
}

static void _ClientsRemove(int i) {
  MorguSockClose(_clients[i].Socket);
  _clients[i].Socket = MORGUSOCK_INVALID;
  _clients[i].Address[0] = '\0';
}

void LaunchConnectionHandler(void) {
  MorguSocket listener;
  char errbuf[256];

  if (MorguSockInit() != 0) {
    fprintf(stderr, "Socket initialization failed\n");
    return;
  }

  listener = MorguSockListenTCP("5000", 16);
  if (listener == MORGUSOCK_INVALID) {
    fprintf(stderr, "Listener initialization failed: %s\n",
            MorguSockStringError(MorguSockErrNo(), errbuf, sizeof errbuf));
    MorguSockClean();
    return;
  }

  printf("Initialized on port 5000\n");

  _ClientsInit();

  for (;;) {
    fd_set readfds;
    struct timeval timeout;
    MorguSocket maxfd = listener;
    int ready;

    // select() modifies our set, so rebuild it on every iteration
    FD_ZERO(&readfds);          // clear the set
    FD_SET(listener, &readfds); // add the listener socket

    for (int i = 0; i < MORGUNOV_MAX_CONNECTIONS; i++) {
      if (_clients[i].Socket != MORGUSOCK_INVALID) {
        FD_SET(_clients[i].Socket, &readfds);
        if (_clients[i].Socket > maxfd) {
          maxfd = _clients[i].Socket;
        }
      }
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    ready = select((int)maxfd + 1, &readfds, NULL, NULL, &timeout);

    if (ready < 0) {
#ifndef _WIN32
      // interrupted by a signal
      if (MorguSockErrNo() == EINTR)
        continue;
#endif

      fprintf(stderr, "socket selection failed: %s\n",
              MorguSockStringError(MorguSockErrNo(), errbuf, sizeof errbuf));
      break;
    }

    if (ready == 0) {
      // timeout
      continue;
    }

    if (FD_ISSET(listener, &readfds)) {
      // new incoming connection
      char host[64];
      MorguSocket c = MorguSockAcceptFrom(listener, host, sizeof host);
      if (c != MORGUSOCK_INVALID) {
        if (_ClientsAdd(c) < 0) {
          const char* message = "server full";
          MorguSockSend(c, message, strlen(message));
          MorguSockClose(c);
          printf("Rejected %s: server full\n", host);
        } else {
          MorguSockSetNoDelay(c);
          printf("Connected %s\n", host);
        }
      }
    }

    // data or disconnect on existing client
    for (int i = 0; i < MORGUNOV_MAX_CONNECTIONS; i++) {
      if (_clients[i].Socket == MORGUSOCK_INVALID)
        continue;
      if (!FD_ISSET(_clients[i].Socket, &readfds))
        continue;

      char buf[1024];
      int n;

      n = MorguSockReceive(_clients[i].Socket, buf, sizeof buf);
      if (n > 0) {
        printf("%s sent %d bytes\n", _clients[i].Address, n);

        // echo back
        if (MorguSockSendAll(_clients[i].Socket, buf, (size_t)n) != 0) {
          printf("send failed to %s\n", _clients[i].Address);
          _ClientsRemove(i);
        }
      } else if (n == 0) {
        printf("disconnected %s\n", _clients[i].Address);
        _ClientsRemove(i);
      } else {
        if (MorguSockWouldBlock())
          continue; // spurious wakeup
        printf("error on %s: %s\n", _clients[i].Address,
               MorguSockStringError(MorguSockErrNo(), errbuf, sizeof errbuf));
        _ClientsRemove(i);
      }
    }
  }

  // clean after yourself
  for (int i = 0; i < MORGUNOV_MAX_CONNECTIONS; i++) {
    if (_clients[i].Socket != MORGUSOCK_INVALID)
      _ClientsRemove(i);
  }
  MorguSockClose(listener);
  MorguSockClean();
}