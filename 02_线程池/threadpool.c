#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"


ThreadPool* threadPoolCreate(int min, int max, int capacity)
{
    ThreadPool *pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (!pool)
    {
        printf("pool malloc failure!\n");
        return NULL;
    }
    do 
    {
        // 任务队列初始化
        pool->taskQ = (Task*)malloc(sizeof(Task) * capacity);
        if (pool->taskQ == NULL) 
        {
            printf("taskQ malloc failure!\n");
            break;
        }
        pool->queueCapacity = capacity;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        // 线程初始化
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->aliveNum = 0;
        pool->exitNum = 0;

        pool->workerIDs = (pthread_t*)malloc(sizeof(pthread_t) * pool->maxNum);
        if (pool->workerIDs == NULL)
        {
            printf("workerIDs malloc failure!\n");
            break;
        }

        // 创建线程
        memset(pool->workerIDs, 0, sizeof(pthread_t) * pool->maxNum);
        pthread_create(&pool->managerID, NULL, manager, pool);
        for (int i = 0; i < pool->minNum; ++i)
        {
            pthread_create(&pool->workerIDs[i], NULL, worker, pool);
        }

        // 初始化条件变量
        if (pthread_cond_init(&pool->queueEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->queueFull, NULL) != 0 ||
            pthread_mutex_init(&pool->poolMutex, NULL) != 0 ||
            pthread_mutex_init(&pool->busyNumMutex, NULL) != 0)
        {
            break;
        }

        pool->shutdown = false;
        return pool;
    }while (0);
    

    if (pool->taskQ) free(pool->taskQ);
    if (pool->workerIDs) free(pool->workerIDs);
    free(pool);
    return NULL;
}


void* worker(void *arg)
{
    ThreadPool *pool = (ThreadPool*)arg;
    while (true)// 工作者一直工作
    {
        pthread_mutex_lock(&pool->poolMutex);
        while (pool->queueSize == 0 && !pool->shutdown)
        {
            // 任务队列中没有任务阻塞
            pthread_cond_wait(&pool->queueEmpty, &pool->poolMutex);
            
            // 线程管理者要销毁线程
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if (pool->aliveNum > pool->minNum)
                {
                    pool->aliveNum--;
                    pthread_mutex_unlock(&pool->poolMutex);
                    threadExit(pool);
                }
            }
        }

        // 线程池关闭
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->poolMutex);
            threadExit(pool);
        }

        // 取出任务
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        // 取出任务后通知阻塞在任务队列已满条件变量处
        pthread_cond_signal(&pool->queueFull);
        pthread_mutex_unlock(&pool->poolMutex);

        pthread_mutex_lock(&pool->busyNumMutex);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->busyNumMutex);

        // 执行任务
        printf("thread %lu start working...\n", pthread_self());
        task.function(task.arg);
        free(task.arg);// 传参时传入堆内存
        printf("thread %lu end working...\n", pthread_self());

        pthread_mutex_lock(&pool->busyNumMutex);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->busyNumMutex);
    }
    return NULL;
}

void* manager(void *arg)
{
    ThreadPool *pool = (ThreadPool*)arg;
    // 线程不关闭时管理者一直检车是否需要添加或销毁线程
    while (!pool->shutdown)
    {
        sleep(3);// 每三秒检测是否需要往线程池添加或销毁线程
                 
        pthread_mutex_lock(&pool->poolMutex);
        int queueSize = pool->queueSize;
        int aliveNum = pool->aliveNum;
        pthread_mutex_unlock(&pool->poolMutex);

        pthread_mutex_lock(&pool->busyNumMutex);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->busyNumMutex);

        // 添加线程 
        if (queueSize > aliveNum && aliveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->poolMutex);
            int counter = 0;
            // 每次添加两个线程
            for (int i = 0; i < pool->maxNum && counter < 2 && pool->aliveNum < pool->maxNum; i++)
            {
                if (pool->workerIDs[i] == 0)
                {
                    pthread_create(&pool->workerIDs[i], NULL, worker, pool);
                    counter++;
                    pool->aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->poolMutex);
        }

        // 销毁线程
        if (busyNum * 2 < aliveNum && aliveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->poolMutex);
            pool->exitNum = 2;
            pthread_mutex_unlock(&pool->poolMutex);

            // 通知阻塞在任务队列为空处的空闲线程，让其自我销毁
            for (int i = 0; i < 2; i++)
            {
                pthread_cond_signal(&pool->queueEmpty);
            }
        }
    }
    return NULL;
}

int addTask(ThreadPool *pool, void(*taskfunc)(void*), void *arg)
{
    pthread_mutex_lock(&pool->poolMutex);
    while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        // 等待工作者取出任务的通知
        pthread_cond_wait(&pool->queueFull, &pool->poolMutex);
    }

    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->poolMutex);
        return -1;
    }

    // 添加任务
    pool->taskQ[pool->queueRear].function = taskfunc;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    // 通知阻塞在任务队列为空处的线程,让其开始工作
    pthread_cond_signal(&pool->queueEmpty);
    pthread_mutex_unlock(&pool->poolMutex);
    return 0;
}

void threadExit(ThreadPool *pool)
{
    // 获取当前线程id
    pthread_t curtid = pthread_self();
    for (int i = 0; i < pool->maxNum; i++)
    {
        if (pool->workerIDs[i] == curtid)
        {
            // 使保存要退出线程id数组对应位置清零,使管理者方便添加新线程
            pool->workerIDs[i] = 0;
            printf("threadExit is called thread:%lu exiting...\n", pthread_self());
            break;
        }
    }
    pthread_exit(NULL);
}


// 销毁线程池
int threadPoolDestroy(ThreadPool *pool)
{
    printf("enter threadPoolDestroy\n");
    if (pool == NULL) 
    {
        return -1;
    }

    printf("shutdown pool\n");
    pool->shutdown = true;
    
    printf("recycle manager\n");
    pthread_join(pool->managerID, NULL);
    for (int i = 0; i < pool->aliveNum; i++)
    {
        pthread_cond_signal(&pool->queueEmpty);
    }

    printf("heap free\n");
    if (pool->taskQ) free(pool->taskQ);
    if (pool->workerIDs) free(pool->workerIDs);

    pthread_cond_destroy(&pool->queueEmpty);
    pthread_cond_destroy(&pool->queueFull);
    pthread_mutex_destroy(&pool->busyNumMutex);
    pthread_mutex_destroy(&pool->poolMutex);

    printf("pool free\n");
    free(pool);
    printf("threadPoolDestroy() called \n");
    return 0;
}
