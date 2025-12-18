#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARRAY_SIZE 128

struct shared_data {
  sem_t sem;
  char array[ARRAY_SIZE];
  unsigned long counter;
} g;

void *writer_thread(void *arg) {
  (void)arg;
  for (;;) {
    if (sem_wait(&g.sem) != 0) {
      perror("sem_wait");
      break;
    }
    
    g.counter++;
    snprintf(g.array, ARRAY_SIZE, "Record #%lu", g.counter);
    
    sem_post(&g.sem);
    sleep(1);
  }
  return NULL;
}

void *reader_thread(void *arg) {
  (void)arg;
  char local[ARRAY_SIZE];
  for (;;) {
    if (sem_wait(&g.sem) != 0) {
      perror("sem_wait");
      break;
    }
    
    strncpy(local, g.array, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';
    
    sem_post(&g.sem);
    
    printf("Reader tid=%lu, array content: \"%s\"\n",
           (unsigned long)pthread_self(), local);
    fflush(stdout);
    
    usleep(500000);
  }
  return NULL;
}

int main() {
  memset(&g, 0, sizeof(g));
  
  if (sem_init(&g.sem, 0, 1) != 0) {
    perror("sem_init");
    return EXIT_FAILURE;
  }
  
  pthread_t writer, reader;
  
  if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
    perror("pthread_create writer");
    sem_destroy(&g.sem);
    return EXIT_FAILURE;
  }
  
  if (pthread_create(&reader, NULL, reader_thread, NULL) != 0) {
    perror("pthread_create reader");
    sem_destroy(&g.sem);
    return EXIT_FAILURE;
  }
  
  pthread_join(writer, NULL);
  pthread_join(reader, NULL);
  
  sem_destroy(&g.sem);
  return 0;
}
