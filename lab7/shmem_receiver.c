#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SHM_NAME "/lab7_shm"

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
  int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
  if (fd == -1) {
    fprintf(stderr, "Receiver: shared memory not found, start sender first.\n");
    return EXIT_FAILURE;
  }

  struct payload *data =
      mmap(NULL, sizeof(struct payload), PROT_READ, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) die("mmap");
  close(fd);

  for (;;) {
    time_t now = time(NULL);
    printf("Receiver pid=%d time=%lld | got: seq=%llu ts=%lld pid=%d text=%s\n",
           getpid(), (long long)now, (unsigned long long)data->seq,
           (long long)data->ts, (int)data->pid, data->text);
    fflush(stdout);
    sleep(1);
  }

  munmap(data, sizeof(*data));
  return 0;
}

