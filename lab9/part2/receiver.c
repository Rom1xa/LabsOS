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
  if (key == -1) {
    perror("ftok");
    return EXIT_FAILURE;
  }

  sem_id = semget(key, 1, 0666);
  if (sem_id == -1) {
    fprintf(stderr, "Receiver: semaphore not found, start sender first.\n");
    return EXIT_FAILURE;
  }

  shm_id = shmget(key, sizeof(struct payload), 0666);
  if (shm_id == -1) {
    fprintf(stderr, "Receiver: shared memory not found, start sender first.\n");
    return EXIT_FAILURE;
  }

  data = shmat(shm_id, NULL, SHM_RDONLY);
  if (data == (void *)-1) die("shmat");

  for (;;) {
    sem_wait_op(sem_id);

    time_t now = time(NULL);
    pid_t my_pid = getpid();
    pid_t sender_pid = data->pid;
    time_t sender_time = data->timestamp;
    char received_msg[256];
    strncpy(received_msg, data->message, sizeof(received_msg) - 1);
    received_msg[sizeof(received_msg) - 1] = '\0';

    sem_post_op(sem_id);

    printf("Receiver: my_time=%lld my_pid=%d | received: pid=%d time=%lld message=\"%s\"\n",
           (long long)now, my_pid, sender_pid, (long long)sender_time, received_msg);
    fflush(stdout);

    sleep(1);
  }

  cleanup();
  return 0;
}
