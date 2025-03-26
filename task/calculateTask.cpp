#include "calculateTask.h"

calculateTask::calculateTask(int start, int end):start(start),end(end)
{

}

calculateTask::~calculateTask()
{

}

void calculateTask::execute()
{
    int sum = 0;
    for(int i = start; i <= end; i++)
    {
        sum += i;
    }
    std::cout << "The sum from " << start << " to " << end << " is " << sum << std::endl;
}