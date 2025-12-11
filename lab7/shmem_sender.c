#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SHM_NAME "/lab7_shm"
#define SEM_NAME "/lab7_prod_lock"

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

static int shm_fd = -1;
static sem_t *sem_lock = SEM_FAILED;
static struct payload *data = NULL;

static void cleanup(void) {
  if (data) munmap(data, sizeof(*data));
  if (shm_fd >= 0) close(shm_fd);
  if (sem_lock != SEM_FAILED) {
    sem_post(sem_lock);
    sem_close(sem_lock);
  }
}

static void on_sigint(int sig) {
  (void)sig;
  cleanup();
  _exit(EXIT_SUCCESS);
}

int main() {
  struct sigaction sa = {.sa_handler = on_sigint};
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  sem_lock = sem_open(SEM_NAME, O_CREAT, 0666, 1);
  if (sem_lock == SEM_FAILED) die("sem_open");
  if (sem_trywait(sem_lock) == -1) {
    if (errno == EAGAIN) {
      fprintf(stderr, "Sender already running (lock held).\n");
      sem_close(sem_lock);
      return EXIT_FAILURE;
    }
    die("sem_trywait");
  }

  shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) die("shm_open");
  if (ftruncate(shm_fd, sizeof(struct payload)) == -1) die("ftruncate");

  data = mmap(NULL, sizeof(struct payload), PROT_READ | PROT_WRITE, MAP_SHARED,
              shm_fd, 0);
  if (data == MAP_FAILED) die("mmap");

  memset(data, 0, sizeof(*data));

  unsigned long long seq = 0;
  for (;;) {
    time_t now = time(NULL);
    data->pid = getpid();
    data->ts = now;
    data->seq = seq++;
    snprintf(data->text, sizeof(data->text), "from sender pid=%d", getpid());
    printf("Sender wrote seq=%llu ts=%lld\n",
           (unsigned long long)data->seq, (long long)data->ts);
    fflush(stdout);
    sleep(1);
  }

  cleanup();
  return 0;
}

