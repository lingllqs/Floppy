#include "threadpool.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void func(void *arg)
{
    int num = *(int *)arg;
    printf("thread %lu is working, number = %d\n", pthread_self(), num);
    sleep(rand() % 3);
}
int main()
{
    ThreadPool *pool = threadPoolCreate(3, 10, 100);
    for (int i = 0; i < 50; i++)
    {
        int *num = (int *)malloc(sizeof(int));
        *num = i + 50;
        addTask(pool, func, num);
    }

    getchar();
    threadPoolDestroy(pool);
    printf("-------------------------\n");
    return 0;
}
