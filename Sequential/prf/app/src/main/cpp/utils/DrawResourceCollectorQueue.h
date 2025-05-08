#ifndef DrawResourceCollectorQueue_H
#define DrawResourceCollectorQueue_H

#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>

#include "../engine2d/DrawResource.h"
#include "../drawTaskContainer/DrawTaskList.h"

class DrawResourceCollectorQueue
{
private:
    /**
     * 队列中的顺序依然从小任务id到大任务id
     * 参考 bool DrawResource::operator>(const DrawResource& dr) const
     */
    std::priority_queue<DrawResource, std::vector<DrawResource>, std::greater<DrawResource>> queue_;
    std::mutex mtx_;
    std::condition_variable itemAdded_;
    uint32_t priorityReturned_;
    DrawTaskList *drawTaskList_ = nullptr; // 含有bbox信息，判断是否可以提前收集
    std::vector<bool> doneList_; // 完成列表，与drawTaskList_一一对应，提前完成的会在此标记

public:
    DrawResourceCollectorQueue();

    uint32_t getSize();

    void reset(DrawTaskList *drawTaskList);

    void produce(DrawResource const &item);

    void produce(DrawResource &&item);

    DrawResource consume();
};

#endif