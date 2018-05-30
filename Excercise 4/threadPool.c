#include "threadPool.h"

void error() {
    write(2, SYS_CALL_ERROR, strlen(SYS_CALL_ERROR));
    exit(ERROR);
}

ThreadPool *tpCreate(int numOfThreads) {
    int i;
    if (numOfThreads <= 0)
        return NULL;

    // Create thread pool
    ThreadPool *pool = (ThreadPool *) calloc(sizeof(ThreadPool), 1);
    if (!pool)
        error();

    // Initialize all fields
    pool->state = ONLINE;
    pool->pool_size = numOfThreads;
    pool->queue = osCreateQueue();;
    if (pthread_mutex_init(&pool->mutex, NULL) != 0)
        error();

    if (pthread_cond_init(&pool->cond, NULL) != 0)
        error();

    pool->threads = (pthread_t *) calloc(sizeof(pthread_t), (size_t) numOfThreads);
    if (!pool->threads)
        error();

    // Start workers
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, execute, (void *) pool) != 0) {
            tpDestroy(pool, 0);
            error();
        }
    }
    return pool;
}

int tpInsertTask(ThreadPool *pool, void (*computeFunc)(void *), void *param) {
    if (pool->state != ONLINE)
        return ERROR; // TP is shutting down - new tasks are not allowed

    task_t *task = (task_t *) calloc(sizeof(task_t), 1);
    if (!task)
        error();

    task->computeFunc = computeFunc;
    task->args = param;
    pthread_mutex_lock(&(pool->mutex));

    osEnqueue(pool->queue, task);
    if (pthread_cond_broadcast(&(pool->cond)) != 0)
        error();
    pthread_mutex_unlock(&(pool->mutex));

    return 0;
}

static void *execute(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;
    while (pool->state != HARD_SHUTDOWN && (!osIsQueueEmpty(pool->queue) || pool->state == ONLINE)) {
        pthread_mutex_lock(&pool->mutex);
        while (osIsQueueEmpty(pool->queue) && pool->state == ONLINE)
            pthread_cond_wait(&(pool->cond), &(pool->mutex));

        task_t *task = (task_t *) osDequeue(pool->queue);
        pthread_mutex_unlock(&pool->mutex);
        if (!task)
            continue;
        ((task->computeFunc))(task->args);
        free(task);
    }
    pthread_mutex_unlock(&(pool->mutex));
    pthread_exit(NULL);
}

void tpDestroy(ThreadPool *pool, int shouldWaitForTasks) {
    int i;
    pthread_mutex_lock(&pool->mutex);
    if (shouldWaitForTasks != 0)
        pool->state = SOFT_SHUTDOWN;
    else
        pool->state = HARD_SHUTDOWN;

    pthread_cond_broadcast(&(pool->cond));
    pthread_mutex_unlock(&pool->mutex);

    for (i = 0; i < pool->pool_size; i++)
        pthread_join(pool->threads[i], NULL); // Join all threads

    free(pool->threads);
    osDestroyQueue(pool->queue);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}
