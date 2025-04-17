// task.h
#ifndef TASK_H
#define TASK_H

#include <iostream>
#include "../json/jsonhelper.h"
// 抽象基类 Task
class Task {
public:
    // 虚析构函数，确保派生类的析构函数被正确调用
    virtual ~Task() {}

    // 纯虚函数，派生类需要实现
    virtual void execute() = 0;

};

#endif // TASK_H
