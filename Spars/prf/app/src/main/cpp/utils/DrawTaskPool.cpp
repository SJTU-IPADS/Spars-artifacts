//
// Created by richardwu on 11/25/24.
//

#include "DrawTaskPool.h"
#include "../renderWorker/RenderWorkerThread.h"

void DrawTaskPool::reset(DrawTaskList *drawTaskList) {

    std::unique_lock<std::mutex> lk(mtx_);
    readPos_.store(0);

    uint32_t taskNum = drawTaskList->getTaskNum();
    drawTaskPool_.resize(taskNum);
    for(int i = 0; i < taskNum; i++) {
        drawTaskPool_[i] = drawTaskList->getDrawTask(i).get();
    }

    canStartFrame_.notify_all();
}

DrawTask* DrawTaskPool::getNextDrawTask() {
    // 自增readPos_并读取自增前的绘制任务
    uint32_t assignedIdx = readPos_.fetch_add(1);
    if (assignedIdx < drawTaskPool_.size()) {
        return drawTaskPool_[assignedIdx]; // 快速路径，无锁
    } else {
        // 等待下一帧
        std::unique_lock<std::mutex> lk(mtx_);
        assignedIdx = readPos_.fetch_add(1);
        while (assignedIdx >= drawTaskPool_.size()) {
            canStartFrame_.wait(lk);
            assignedIdx = readPos_.fetch_add(1);
        }

        return drawTaskPool_[assignedIdx];
    }

}

void DrawTaskPool::resetDummy(uint32_t threadCount) {
    // 发线程数量的 RENDER_WORKER_THREAD_DEAD_MARK，每个线程收到一次后便终结，因此每个线程只会收到一次
    std::unique_lock<std::mutex> lk(mtx_);
    readPos_.store(0);
    drawTaskPool_.clear();

    for(int i = 0; i < threadCount; i++) {
        // a dummy draw task
        std::vector<Rect> rects;
        std::vector<Paint> paints;
        Rect boundingBox = Rect::MakeXYWH(0, 0, 0, 0);
        auto *drawTask = new RectsDrawTask(RENDER_WORKER_THREAD_DEAD_MARK, boundingBox, rects, paints);
        drawTaskPool_.push_back(drawTask);
    }

    canStartFrame_.notify_all();
}