#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READER_COUNT 10
#define ITERATIONS 20

struct shared_data {
  pthread_mutex_t mtx;
  unsigned long counter;
  char buf[64];
  bool done;
} g = {PTHREAD_MUTEX_INITIALIZER, 0, {0}, false};

void *writer_thread(void *arg) {
  (void)arg;
  for (unsigned i = 0; i < ITERATIONS; ++i) {
    if (pthread_mutex_lock(&g.mtx) != 0) {
      perror("mutex lock");
      break;
    }
    g.counter++;
    snprintf(g.buf, sizeof(g.buf), "write #%lu", g.counter);
    pthread_mutex_unlock(&g.mtx);
    usleep(200000);
  }
  pthread_mutex_lock(&g.mtx);
  g.done = true;
  pthread_mutex_unlock(&g.mtx);
  return NULL;
}

void *reader_thread(void *arg) {
  (void)arg;
  char local[64];
  for (;;) {
    pthread_mutex_lock(&g.mtx);
    unsigned long c = g.counter;
    bool done = g.done;
    strncpy(local, g.buf, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';
    pthread_mutex_unlock(&g.mtx);
    printf("reader %lu sees counter=%lu buf=\"%s\"\n",
           (unsigned long)pthread_self(), c, local);
    if (done)
      break;
    usleep(150000);
  }
  return NULL;
}

int main() {
  pthread_t writer;
  pthread_t readers[READER_COUNT];

  if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
    perror("pthread_create writer");
    return EXIT_FAILURE;
  }
  for (int i = 0; i < READER_COUNT; ++i) {
    if (pthread_create(&readers[i], NULL, reader_thread, NULL) != 0) {
      perror("pthread_create reader");
      return EXIT_FAILURE;
    }
  }

  pthread_join(writer, NULL);
  for (int i = 0; i < READER_COUNT; ++i)
    pthread_join(readers[i], NULL);
  return 0;
}
