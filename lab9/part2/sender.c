#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct payload {
  pid_t pid;
  time_t timestamp;
  char message[256];
};

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

static int shm_id = -1;
static int sem_id = -1;
static struct payload *data = NULL;

static void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void cleanup(void) {
  if (data) {
    shmdt(data);
    data = NULL;
  }
  if (shm_id >= 0) {
    shmctl(shm_id, IPC_RMID, NULL);
    shm_id = -1;
  }
  if (sem_id >= 0) {
    union semun arg;
    semctl(sem_id, 0, IPC_RMID, arg);
    sem_id = -1;
  }
}

static void on_sigint(int sig) {
  (void)sig;
  cleanup();
  _exit(EXIT_SUCCESS);
}

static void sem_wait_op(int semid) {
  struct sembuf op = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
  if (semop(semid, &op, 1) == -1) {
    die("semop wait");
  }
}

static void sem_post_op(int semid) {
  struct sembuf op = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
  if (semop(semid, &op, 1) == -1) {
    die("semop post");
  }
}

int main() {
  struct sigaction sa = {.sa_handler = on_sigint};
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  key_t key = ftok(".", 'A');
  if (key == -1) die("ftok");

  sem_id = semget(key, 1, IPC_CREAT | 0666);
  if (sem_id == -1) die("semget");

  union semun arg;
  arg.val = 1;
  if (semctl(sem_id, 0, SETVAL, arg) == -1) die("semctl SETVAL");

  shm_id = shmget(key, sizeof(struct payload), IPC_CREAT | 0666);
  if (shm_id == -1) die("shmget");

  data = shmat(shm_id, NULL, 0);
  if (data == (void *)-1) die("shmat");

  memset(data, 0, sizeof(*data));

  for (;;) {
    sem_wait_op(sem_id);

    time_t now = time(NULL);
    data->pid = getpid();
    data->timestamp = now;
    snprintf(data->message, sizeof(data->message),
             "Time: %s, PID: %d", ctime(&now), getpid());
    data->message[strlen(data->message) - 1] = '\0';

    sem_post_op(sem_id);

    printf("Sender: sent time=%lld pid=%d\n", (long long)now, getpid());
    fflush(stdout);

    sleep(3);
  }

  cleanup();
  return 0;
}
