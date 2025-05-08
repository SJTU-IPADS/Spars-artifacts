//
// Created by richardwu on 11/25/24.
//

#include "DrawResourceCollectorQueue.h"

DrawResourceCollectorQueue::DrawResourceCollectorQueue() {
    priorityReturned_ = 0;
};

uint32_t DrawResourceCollectorQueue::getSize()
{
    std::unique_lock<std::mutex> lk(mtx_);
    return queue_.size();
}

void DrawResourceCollectorQueue::reset(DrawTaskList *drawTaskList) // 调用时务必确保queue_为空
{
    std::unique_lock<std::mutex> lk(mtx_);
    drawTaskList_ = drawTaskList;
    doneList_.assign(drawTaskList->getTaskNum(), false);
    priorityReturned_ = 0;
}

void DrawResourceCollectorQueue::produce(DrawResource const &item)
{
    std::unique_lock<std::mutex> lk(mtx_);
    queue_.push(item);
    itemAdded_.notify_one();
}

void DrawResourceCollectorQueue::produce(DrawResource &&item)
{
    std::unique_lock<std::mutex> lk(mtx_);
    queue_.push(std::move(item));
    itemAdded_.notify_one();
}

DrawResource DrawResourceCollectorQueue::consume()
{
    std::unique_lock<std::mutex> lk(mtx_);

    while (queue_.empty() || !queue_.top().canCollect(priorityReturned_, drawTaskList_))
    {
        itemAdded_.wait(lk);
    }

    DrawResource item = queue_.top();

    // 已收集
    doneList_[item.taskId_] = true;

    // 顺序收集情况
    if (item.taskId_ == priorityReturned_) {

        // 直到加到第一个为false的任务
        while(priorityReturned_ < doneList_.size() && doneList_[priorityReturned_]) {
            priorityReturned_++;
        }
    }

    queue_.pop();

    return item;
}

