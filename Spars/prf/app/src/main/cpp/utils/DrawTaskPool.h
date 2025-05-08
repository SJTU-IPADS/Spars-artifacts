//
// Created by richardwu on 11/25/24.
//

#ifndef PRF_DRAWTASKPOOL_H
#define PRF_DRAWTASKPOOL_H

#include <vector>
#include "../engine2d/DrawTask.h"

/**
 * 任务池：工作线程读取时无锁
 */
class DrawTaskPool {
private:
    std::vector<DrawTask*> drawTaskPool_;
    std::atomic_uint32_t readPos_{0};

    // 唤醒这一帧的工作线程工作
    std::mutex mtx_;
    std::condition_variable canStartFrame_;

public:
    /**
     * 重置发送，并将任务任务放置好，最后通知工作线程开始工作。该步骤必须在重置收集之后
     * @param drawTaskList
     */
    void reset(DrawTaskList *drawTaskList);

    /**
     * 发送用于终结工作线程的dummy任务
     * @param threadCount
     */
    void resetDummy(uint32_t threadCount);

    DrawTask* getNextDrawTask();
};


#endif //PRF_DRAWTASKPOOL_H
