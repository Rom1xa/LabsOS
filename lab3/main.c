#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void cleanup_handler(void) {
  printf("--- [atexit handler]: Процесс с PID %d завершает работу. ---\n",
         getpid());
}

void sigint_handler(int signum) {
  (void)signum;
  char msg[] =
      "--- [signal handler]: Получен сигнал SIGINT (Ctrl+C). Игнорирую. ---\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

void sigterm_handler(int signum, siginfo_t *info, void *context) {
  (void)context;
  (void)info;
  char msg[120];
  sprintf(msg,
          "--- [sigaction handler]: Получен сигнал SIGTERM (%d). Завершаю "
          "работу. ---\n",
          signum);
  write(STDOUT_FILENO, msg, strlen(msg));
  exit(1);
}

int main() {
  printf("Родительский процесс запущен с PID: %d, PPID: %d\n", getpid(),
         getppid());

  if (atexit(cleanup_handler) != 0) {
    perror("Ошибка регистрации atexit()");
    exit(1);
  }

  if (signal(SIGINT, sigint_handler) == SIG_ERR) {
    perror("Ошибка установки обработчика SIGINT");
    exit(1);
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = sigterm_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGTERM, &sa, NULL) == -1) {
    perror("Ошибка установки обработчика SIGTERM");
    exit(1);
  }

  printf("Обработчики сигналов установлены. Для тестирования используйте:\n");
  printf("- Ctrl+C для SIGINT\n");
  printf("- kill -TERM %d для SIGTERM\n", getpid());

  FILE *f = fopen("pid.txt", "w");
  if (f) {
    fprintf(f, "%d", getpid());
    fclose(f);
    printf("PID записан в файл pid.txt\n");
  }

  printf("\nВызываем fork()...\n");

  pid_t child_pid = fork();

  if (child_pid == -1) {
    perror("Ошибка fork()");
    exit(1);
  } else if (child_pid == 0) {
    printf("Это дочерний процесс.\n");
    printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
    printf("Дочерний процесс: засыпаю на 5 секунд...\n");
    sleep(5);
    printf("Дочерний процесс: завершаю работу с кодом 42.\n");
    exit(42);
  } else {
    printf("Это родительский процесс.\n");
    printf("Родительский процесс: PID = %d, PPID = %d\n", getpid(), getppid());
    printf("Создан дочерний процесс с PID: %d\n", child_pid);

    int status;
    printf("Родительский процесс: ожидаю завершения дочернего процесса...\n");
    pid_t waited_pid = wait(&status);

    if (waited_pid == -1) {
      perror("Ошибка wait()");
      exit(1);
    }

    if (WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      printf("Родительский процесс: дочерний процесс завершился с кодом %d.\n",
             exit_code);
    } else if (WIFSIGNALED(status)) {
      int signal_num = WTERMSIG(status);
      printf(
          "Родительский процесс: дочерний процесс был завершен сигналом %d.\n",
          signal_num);
    }

    printf("Родительский процесс: засыпаю на 10 секунд...\n");
    sleep(10);
    printf("Родительский процесс завершает работу\n");
  }

  printf("Процесс %d завершается нормально\n", getpid());

  return 0;
}
