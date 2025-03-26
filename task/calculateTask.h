#ifndef __CALCULATETASK_H__
#define __CALCULATETASK_H__

#include "task.h"

class calculateTask:public Task{
    public:
        // 构造函数
        calculateTask(int start, int end);

        // 析构函数
        ~calculateTask();

        void execute();
    private:
    int start;
    int end;
};
#endif // __CALCULATETASK_H__
