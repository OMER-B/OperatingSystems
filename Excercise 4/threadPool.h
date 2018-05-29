#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "osqueue.h"

typedef enum state { OFFLINE, ONLINE, HARD_SHUTDOWN, SOFT_SHUTDOWN } state;
typedef enum bool { false, true };
typedef struct thread_pool {
  int pool_size;
  int queue_size;
  int active_threads;
  int tasks_waiting;
  pthread_t *threads;
  OSQueue *queue;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  state state;
} ThreadPool;

typedef struct task_t {
  void (*computeFunc)(void *);
  void *args;
} task_t;

static void *execute(void *arg);

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *pool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

#endif
