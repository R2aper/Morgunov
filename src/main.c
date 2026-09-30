#include <stdio.h>
#include <string.h>

#include "network/morgusock.h"

int sockserver() {
  char err[128];

  if(MorguSockInit() != 0) { fprintf(stderr, "init failed"); return 1; }

  MorguSocket server = MorguSockListenTCP("8080", 16);
  if(server == MORGUSOCK_INVALID) {
    fprintf(stderr, MorguSockStringError(MorguSockErrNo(), err, sizeof err));
    MorguSockClean();
    return 1;
  }

  printf("listening");

  for(;;) {
    char peer[128];
    MorguSocket client = MorguSockAcceptFrom(server, peer, sizeof peer);
    if(client == MORGUSOCK_INVALID) {
      fprintf(stderr, MorguSockStringError(MorguSockErrNo(), err, sizeof err));
      continue;
    }
    
    printf("client connected: %s\n", peer);

    char buf[1024];
    int n;
    while((n == MorguSockReceive(client, buf, sizeof buf)) > 0) {
      if(MorguSockSendAll(client, buf, (size_t)n) != 0) break;
    }

    printf("client disconnected");
    MorguSockClose(client);
  }

  MorguSockClose(server);
  MorguSockClean();
  return 0;
}

int sockclient() {
  char err[128];

  if(MorguSockInit() != 0) { fprintf(stderr, "init failed"); return 1; }

  MorguSocket s = MorguSockConnectTCP("localhost", "8080");
  if (s == MORGUSOCK_INVALID) {
      fprintf(stderr, "connect to localhost failed: %s\n",
              MorguSockStringError(MorguSockErrNo(), err, sizeof err));
      MorguSockClean();
      return 1;
  }

  printf("connected");

  char line[512];
  while (fgets(line, sizeof line, stdin) != NULL) {
    size_t len = strlen(line);

    if (MorguSockSendAll(s, line, len) != 0) {
        fprintf(stderr, "send failed\n");
        break;
    }

    /* The server echoes exactly what we sent, so read exactly len bytes back. */
    char reply[512];
    int r = MorguSockReceive(s, reply, len);
    if (r != 0) {
        fprintf(stderr, r == 1 ? "server closed connection\n" : "recv failed\n");
        break;
    }
    reply[len] = '\0';
    printf("echo: %s", reply);
  }

  MorguSockClose(s);
  MorguSockClean();
  return 0;
}

int main(int argc, char* argv[]) {

  if(argc != 2) return -1;
  int c;

  if(strcmp(argv[1], "server") == 0) {
    c = sockserver();
  }
  else if(strcmp(argv[1], "client") == 0) {
    c = sockclient();
  }

  printf("%i\n", c);
  return 0;
}
