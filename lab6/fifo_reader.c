#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/lab6_fifo"

static void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int main(void) {
  if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) die("mkfifo");

  int fd = open(FIFO_PATH, O_RDONLY);
  if (fd < 0) die("open fifo for read");

  char buf[256] = {0};
  ssize_t r = read(fd, buf, sizeof(buf) - 1);
  if (r < 0) die("read fifo");
  close(fd);

  time_t now = time(NULL);
  printf("Reader time: %lld, received: %s\n", (long long)now, buf);
  return 0;
}

