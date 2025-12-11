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

int main(int argc, char **argv) {
  const char *payload = (argc > 1) ? argv[1] : "fifo message";

  if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) die("mkfifo");

  time_t now = time(NULL);
  char msg[256];
  snprintf(msg, sizeof(msg), "from writer pid=%d time=%lld text=%s",
           (int)getpid(), (long long)now, payload);

  sleep(10); 

  int fd = open(FIFO_PATH, O_WRONLY);
  if (fd < 0) die("open fifo for write");

  size_t len = strlen(msg);
  if (write(fd, msg, len) != (ssize_t)len) die("write fifo");
  close(fd);
  printf("Writer sent: %s\n", msg);
  return 0;
}

