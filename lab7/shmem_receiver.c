#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SHM_KEY 0x1234

struct payload {
  pid_t pid;
  time_t ts;
  unsigned long long seq;
  char text[128];
};

static void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int main() {
  int shm_id = shmget(SHM_KEY, sizeof(struct payload), 0666);
  if (shm_id == -1) {
    fprintf(stderr, "Receiver: shared memory not found, start sender first.\n");
    return EXIT_FAILURE;
  }

  struct payload *data = shmat(shm_id, NULL, SHM_RDONLY);
  if (data == (void *)-1) die("shmat");

  for (;;) {
    time_t now = time(NULL);
    printf("Receiver pid=%d time=%lld | got: seq=%llu ts=%lld pid=%d text=%s\n",
           getpid(), (long long)now, (unsigned long long)data->seq,
           (long long)data->ts, (int)data->pid, data->text);
    fflush(stdout);
    sleep(1);
  }

  shmdt(data);
  return 0;
}

