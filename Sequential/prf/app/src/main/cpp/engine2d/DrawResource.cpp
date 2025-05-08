//
// Created by richardwu on 10/31/24.
//

#include "DrawResource.h"
#include "rects/Rect.h"

// 以下方法为插入OrderedPriorityQueue中必须的运算符
bool DrawResource::operator>(const DrawResource& dr) const
{
    return taskId_ > dr.taskId_;
}
//bool DrawResource::operator!=(uint32_t i) const
//{
//    return taskId_ != i;
//}


bool DrawResource::canCollect(uint32_t currTaskDone, DrawTaskList *drawTaskList) const {
    // 最常见的情况，当前任务就是第一个需要收集的任务
    if (currTaskDone == taskId_) {
        return true;
    }

    // 若taskId_（当前完成的任务）太大，则不考虑乱序
    if (currTaskDone < taskId_ - MAX_COLLECT_REORDER) {
        return false;
    }

    // 判断从currTaskDone到taskId_前所有任务的bbox是不是都不冲突
    for (uint32_t i = currTaskDone; i < taskId_; i++) {
        bool overlap = isOverlap(drawTaskList->getDrawTask(i)->boundingBox_, drawTaskList->getDrawTask(taskId_)->boundingBox_);
        if (overlap) {
            return false;
        }
    }

    return true;
}

