#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int main(void) {
  int fds[2];
  if (pipe(fds) == -1) die("pipe");

  pid_t pid = fork();
  if (pid < 0) die("fork");

  if (pid == 0) {
    close(fds[1]);
    char buf[256] = {0};
    ssize_t r = read(fds[0], buf, sizeof(buf) - 1);
    if (r < 0) die("read");
    time_t now = time(NULL);
    printf("Child time: %lld, received: %s\n", (long long)now, buf);
    close(fds[0]);
    exit(EXIT_SUCCESS);
  }

  close(fds[0]);
  time_t now = time(NULL);
  char msg[256];
  snprintf(msg, sizeof(msg), "from parent pid=%d time=%lld", (int)getpid(),
           (long long)now);
  sleep(5); 
  if (write(fds[1], msg, strlen(msg)) < 0) die("write");
  close(fds[1]);
  wait(NULL);
  return 0;
}

