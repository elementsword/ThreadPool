#include "threadpool.h"
ThreadPool::ThreadPool(int size) : poolSize(size), isStop(false)
{
    for (int i = 0; i < poolSize; i++)
    {
        _threads.emplace_back(std::thread(&ThreadPool::workThread, this));
    }
}
ThreadPool::~ThreadPool()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    isStop = true;
    // 唤醒所有线程
    condition.notify_all();
    for (int i = 0; i < poolSize; i++)
    {
        // 等待资源释放退出
        _threads[i].join();
    }
}
// 提交任务到线程池
void ThreadPool::submit(Task *task)
{
    std::unique_lock<std::mutex> lock(queueMutex);
    // 如果线程池已经停止，抛出异常
    if (isStop)
    {
        throw std::runtime_error("ThreadPool is stopped");
    }
    // 将任务添加到队列
    _queue.push(task);
    // 唤醒一个线程
    condition.notify_one();
}
void ThreadPool::workThread()
{
    while (true)
    {
        Task *task = nullptr;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            // 等待任务
            condition.wait(lock, [this] { return !this->_queue.empty() || this->isStop; });
            // 如果线程池已经停止，退出线程
            if (isStop && _queue.empty())
            {
                return;
            }
            // 取出任务
            task = _queue.front();
            _queue.pop();
        }
        // 执行任务
        task->execute();
        delete task;
    }
}