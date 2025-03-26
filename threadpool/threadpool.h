#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include "../task/task.h"
class ThreadPool
{
public:
    ThreadPool(int);
    ~ThreadPool();
private:
    // 线程池大小
    int poolSize;
    //等待队列大小
    int queueSize;
    //线程池是否暂停
    bool isStop;
    //线程池
    std::vector<std::thread> _threads;
    //队列
    std::queue<Task*> _queue;
    //锁(保护队列)
    std::mutex queueMutex;
};

#endif