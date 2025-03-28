#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../task/task.h"
#include "../log/log.h"
class ThreadPool
{
public:
    ThreadPool(int);
    ~ThreadPool();
    // 提交任务到线程池
    void submit(Task* task);
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
    // 条件变量(用于线程同步)
    std::condition_variable condition;
    //工作逻辑
    void workThread();
};

#endif