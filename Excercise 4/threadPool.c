#include "threadPool.h"

inline void error() {
  exit(-1);
}

ThreadPool *tpCreate(int numOfThreads) {
  puts("In tpCreate");

  ThreadPool *thread_pool = (ThreadPool *) calloc(sizeof(ThreadPool), 1);
  if (!thread_pool)
    error();

  thread_pool->state = ONLINE;
  thread_pool->pool_size = numOfThreads;
  thread_pool->queue = osCreateQueue();
  pthread_mutex_init(&thread_pool->mutex, NULL);
  pthread_cond_init(&thread_pool->cond, NULL);
  thread_pool->threads = (pthread_t *) calloc(sizeof(pthread_t), (size_t) numOfThreads);

  if (!thread_pool->threads)
    error();

  int i;
  for (i = 0; i < numOfThreads; i++) {
    if (pthread_create(&(thread_pool->threads[i]), NULL, execute, (void *) thread_pool) != 0) {
      tpDestroy(thread_pool, 0);
      return NULL;
    }
//    thread_pool->pool_size++;
    thread_pool->active_threads++;
  }

  return thread_pool;
}

static void *execute(void *arg) {
  puts("In execute");

  ThreadPool *threadpool = (ThreadPool *) arg;
  task_t *task;
  while (threadpool->state == ONLINE) {
    pthread_mutex_lock(&threadpool->mutex);
    while ((threadpool->tasks_waiting == 0) && (threadpool->state == ONLINE)) {
      pthread_cond_wait(&(threadpool->cond), &(threadpool->mutex));
    }

    if (!osIsQueueEmpty(threadpool->queue)) {
      task = (task_t *) osDequeue(threadpool->queue);
      pthread_mutex_unlock(&threadpool->mutex);
      (*(task->computeFunc))(task->args);
    }
    threadpool->active_threads--;
    pthread_mutex_unlock(&threadpool->mutex);
    pthread_exit(NULL);
    return (NULL);
  }
}

void tpDestroy(ThreadPool *pool, int shouldWaitForTasks) {
  puts("In tpDestroy");
  pthread_mutex_unlock(&(pool->mutex));
  int i;
  if (shouldWaitForTasks == 1) {
    pool->state = SOFT_SHUTDOWN;
    for (i = 0; i < pool->pool_size; i++) {
      pthread_join(pool->threads[i], NULL);

    }
  } else {
    pool->state = HARD_SHUTDOWN;
    osDestroyQueue(pool->queue);
    for (i = 0; i < pool->pool_size; i++) {
      pthread_join(pool->threads[i], NULL);
    }
  }
  pool->state = OFFLINE;
  free(pool->threads);
  free(pool);
}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
  puts("In tpInsertTask");
  if (threadPool->state == SOFT_SHUTDOWN || threadPool->state == HARD_SHUTDOWN)
    return -1;

  task_t *task = (task_t *) calloc(sizeof(task_t), 1);
  if (!task)
    error();

  task->computeFunc = computeFunc;
  task->args = param;
  threadPool->tasks_waiting += 1;
  osEnqueue(threadPool->queue, task);
  ++threadPool->active_threads;
  return 0;
}