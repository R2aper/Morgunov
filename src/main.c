#include <stdio.h>
#include <string.h>

#include "common/dynarray.h"
#include "network/morgusock.h"

int sockclient() {
  char err[128];

  if (MorguSockInit() != 0) {
    fprintf(stderr, "init failed");
    return 1;
  }

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

int main(int argc, char *argv[]) {
  int* testArray = NULL;
  M_DynArrayInit(testArray);

  for(int i = 0; i < 5; i++) {
    M_DynArrayPush(testArray, i);
  }

  M_DynArrayRemove(testArray, 0);
  printf("%i", testArray[0]);
}
