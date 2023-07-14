#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>

// 任务结构体
typedef struct Task {
  void (*function)(void *); // 任务函数
  void *arg;                // 参数
} Task;

typedef struct ThreadPool {
  Task *taskQ;       // 任务队列(环形队列)
  int queueCapacity; // 队列容量
  int queueSize;     // 队列当前任务个数
  int queueFront;    // 队头
  int queueRear;     // 队尾

  pthread_t managerID;          // 管理者线程
  pthread_t *workerIDs;         // 工作这线程
  int minNum;                   // 线程池最小线程数
  int maxNum;                   // 线程池最大线程数
  int busyNum;                  // 正在工作的线程数
  int aliveNum;                 // 线程池当前线程数
  int exitNum;                  // 退出线程（管理者线程使用）
  pthread_cond_t queueEmpty;    // 队列为空条件变量
  pthread_cond_t queueFull;     // 队列为满条件变量
  pthread_mutex_t poolMutex;    // 线程池互斥锁
  pthread_mutex_t busyNumMutex; // 忙碌线程锁

  bool shutdown; // 线程池关闭/打开标志
} ThreadPool;

// 线程池创建
ThreadPool *threadPoolCreate(int min, int max, int capacity);

// 工作者线程
void *worker(void *arg);

// 管理者线程
void *manager(void *arg);

// 添加任务
int addTask(ThreadPool *pool, void (*taskfunc)(void *), void *arg);

// 线程退出
void threadExit(ThreadPool *pool);

// 线程池销毁
int threadPoolDestroy(ThreadPool *pool);

#endif
