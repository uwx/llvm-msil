#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_key_t slotKey;
static pthread_mutex_t counterLock = PTHREAD_MUTEX_INITIALIZER;
static int destructorRuns;
static long total;

static void releaseSlot(void *slot) {
  pthread_mutex_lock(&counterLock);
  destructorRuns++;
  pthread_mutex_unlock(&counterLock);
  free(slot);
}

static void *work(void *argument) {
  long id = (long)argument;
  int *slot = (int *)malloc(sizeof(int));
  *slot = (int)id;
  pthread_setspecific(slotKey, slot);
  for (int i = 0; i < 10000; i++) {
    pthread_mutex_lock(&counterLock);
    total += id;
    pthread_mutex_unlock(&counterLock);
  }
  return (void *)(id * 2);
}

int main() {
  pthread_key_create(&slotKey, releaseSlot);
  pthread_t threads[4];
  for (long i = 0; i < 4; i++)
    pthread_create(&threads[i], nullptr, work, (void *)(i + 1));
  long joined = 0;
  for (int i = 0; i < 4; i++) {
    void *result;
    pthread_join(threads[i], &result);
    joined += (long)result;
  }
  pthread_key_delete(slotKey);
  printf("total %ld joined %ld destructors %d\n", total, joined, destructorRuns);
  return 0;
}
