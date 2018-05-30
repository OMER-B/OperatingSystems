#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "osqueue.h"
#include <string.h>
#include <zconf.h>

#define SYS_CALL_ERROR "Error in system call\n"
#define ERROR           -1

typedef enum state { OFFLINE, ONLINE, HARD_SHUTDOWN, SOFT_SHUTDOWN } state;

/**
 * Struct for the Thread Pool
 * @param pool_size Size of the pool
 * @param threads   Array of threads
 * @param queue     Queue for the pool
 * @param mutex     Mutex
 * @param cond      Condition
 * @param state     Current state of the pool
 */
typedef struct thread_pool {
    int pool_size;
    pthread_t *threads;
    OSQueue *queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    state state;
} ThreadPool;

/**
 * Struct for the task.
 * @param computeFunc   Tasks function,
 * @param args          Arguments for the function.
 */
typedef struct task_t {
    void (*computeFunc)(void *);
    void *args;
} task_t;

/**
 * Write error to fd 2 and exit.
 */
void error();

/**
 * Execute the task.
 * @param arg Thread Pool as void *.
 * @return NULL
 */
static void *execute(void *arg);

/**
 * Creates Thread Pool and initialize all fields.
 * @param numOfThreads Number of threads to allocate.
 * @return Thread Pool
 */
ThreadPool *tpCreate(int numOfThreads);

/**
 * Destroys the Thread Pool.
 * @param pool                  Thread pool to destroy.
 * @param shouldWaitForTasks    Wait for task in queue or not.
 */
void tpDestroy(ThreadPool *pool, int shouldWaitForTasks);

/**
 * Insert a task to the que.
 * @param pool          Thread Pool to add to its queue.
 * @param computeFunc   Function to add.
 * @param param         Arguments for the function.
 * @return -1 if fail, 0 otherwise.
 */
int tpInsertTask(ThreadPool *pool, void (*computeFunc)(void *), void *param);

#endif
