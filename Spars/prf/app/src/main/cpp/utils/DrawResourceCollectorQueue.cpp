//
// Created by richardwu on 11/25/24.
//

#include "DrawResourceCollectorQueue.h"

void DrawResourceCollectorQueue::reset(DrawTaskList *drawTaskList) // 调用时务必确保queue_为空
{
    uint32_t len = drawTaskList->getTaskNum();
    drawResourceQueue_.resize(len); // 不必管原有值

    delete[] statusList_; // delete nullptr是可以的，为nop
    statusList_ = new std::atomic<DrawResourceStatus>[len];
    for (size_t i = 0; i < len; ++i) {
        statusList_[i].store(DrawResourceStatus::RENDERING);
    }

    possibleToCollect_.store(false);
    taskRendered_.store(0);

    drawTaskList_ = drawTaskList;
    priorityReturned_ = 0;
}

void DrawResourceCollectorQueue::produce(const std::shared_ptr<DrawResource>& drawResource)
{
    // 工作线程在插入过程中无锁
    uint32_t taskId = drawResource->taskId_;
    drawResourceQueue_[taskId] = drawResource;
    statusList_[taskId].store(DrawResourceStatus::RENDERED);
    taskRendered_.fetch_add(1);

    // 通过共享锁唤起收集（shared_lock在工作线程之间不会竞争）
    std::shared_lock<std::shared_mutex> lk(mtx_);
    possibleToCollect_.store(true); // 使用原子变量是因为多个producer会写入
    itemAdded_.notify_one();

}

std::shared_ptr<DrawResource> DrawResourceCollectorQueue::consume()
{
    while(true) {

        if (priorityReturned_ >= drawResourceQueue_.size()) {
            throw std::runtime_error("DrawResourceCollectorQueue::consume out of range error!");
        }

        DrawResourceStatus status = statusList_[priorityReturned_].load();

        // 如果头部任务渲染完成，直接返回
        if (status == DrawResourceStatus::RENDERED) {
            statusList_[priorityReturned_].store(DrawResourceStatus::COLLECTED);
            return drawResourceQueue_[priorityReturned_++];
        }
        else if (status == DrawResourceStatus::RENDERING) {
            // 有后续已渲染完成任务，可以考虑提前。在已经堆积太多时不考虑
            uint32_t taskRendered = taskRendered_.load();
            if (taskRendered > priorityReturned_ && taskRendered <= priorityReturned_ + MAX_COLLECT_REORDER) {
                std::vector<Rect> bbxs; // 所有需要检查的bbx
                for (uint32_t i = priorityReturned_; i < priorityReturned_ + MAX_COLLECT_REORDER; i++) {
                    // 提前失败
                    if (i >= drawTaskList_->getTaskNum()) {
                        break;
                    }

                    DrawResourceStatus curr = statusList_[i].load();

                    // 试图让该节点提前收集
                    if (curr == DrawResourceStatus::RENDERED) {
                        bool canCollectNow = true;
                        for(Rect bbx : bbxs) {
                            if (isOverlap(bbx, drawTaskList_->getDrawTask(i)->boundingBox_)) {
                                canCollectNow = false;
                                break; // 一旦有重叠就不能提前收集
                            }
                        }
                        // 可以提前
                        if (canCollectNow) {
                            statusList_[i].store(DrawResourceStatus::COLLECTED);
                            return drawResourceQueue_[i]; // 不必去管priorityReturned_
                        } else {
                            break; // 提前失败
                        }
                    }
                    // 这是个需要被检查的bbx
                    else {
                        bbxs.push_back(drawTaskList_->getDrawTask(i)->boundingBox_);
                    }
                }
                // 提前失败
            }
        } else {
            // 头部任务之前已经COLLECTED
            priorityReturned_++;
            continue;
        }

        { // 锁范围
            std::unique_lock<std::shared_mutex> lk(mtx_);
            while (!possibleToCollect_.load()) {
                itemAdded_.wait(lk);
            }
            possibleToCollect_.store(false);
        }
    }
}

