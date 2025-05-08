#ifndef DrawResourceCollectorQueue_H
#define DrawResourceCollectorQueue_H

#include <thread>
#include <shared_mutex>
#include <memory>
#include <vector>
#include <condition_variable>

#include "../engine2d/DrawResource.h"
#include "../drawTaskContainer/DrawTaskList.h"

#define MAX_COLLECT_REORDER 3

enum DrawResourceStatus : uint8_t
{
    RENDERING,
    RENDERED,
    COLLECTED,
};

class DrawResourceCollectorQueue
{
private:
    std::vector<std::shared_ptr<DrawResource>> drawResourceQueue_;
    std::atomic<DrawResourceStatus>* statusList_ = nullptr; // 与drawResourceQueue_一一对应，提前完成的会在此标记

    uint32_t priorityReturned_; // 时刻维护第一个需要收集的任务
    DrawTaskList *drawTaskList_ = nullptr; // 含有所有bbox信息，判断是否可以提前收集

    std::shared_mutex mtx_; // 共享锁，工作线程之间不发生竞争
    std::condition_variable_any itemAdded_; // 用以唤起收集线程

    std::atomic_uint32_t taskRendered_{0}; // 已经渲染完成的任务
    std::atomic_bool possibleToCollect_{false}; // 存在已渲染未被试图收集的资源

public:

    void reset(DrawTaskList *drawTaskList);

    void produce(const std::shared_ptr<DrawResource>& drawResource);

    std::shared_ptr<DrawResource> consume();
};

#endif