#include "../log/log.h"
#include "../threadpool/threadpool.h"
#include "../task/calculateTask.h"
int main(){
    
    // 创建线程池
    ThreadPool pool(4);
    // 提交任务
    for (int i = 0; i < 10; i++)
    {
        Task *task = new calculateTask(i * 10, (i + 1) * 10 - 1);
        pool.submit(task);
    }
    // 等待所有任务完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}