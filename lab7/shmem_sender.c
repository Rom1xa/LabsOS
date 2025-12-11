#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SHM_KEY 0x1234
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

static int shm_id = -1;
static sem_t *sem_lock = SEM_FAILED;
static struct payload *data = NULL;

static void cleanup(void) {
  if (data) {
    shmdt(data);
    data = NULL;
  }
  if (shm_id >= 0) {
    shmctl(shm_id, IPC_RMID, NULL);
    shm_id = -1;
  }
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

  shm_id = shmget(SHM_KEY, sizeof(struct payload), IPC_CREAT | 0666);
  if (shm_id == -1) die("shmget");

  data = shmat(shm_id, NULL, 0);
  if (data == (void *)-1) die("shmat");

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

