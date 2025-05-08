//
// Created by richardwu on 10/31/24.
//

#ifndef PRF_RENDERWORKERTHREAD_H
#define PRF_RENDERWORKERTHREAD_H

#include <thread>
#include <atomic>
#include <memory>

#include "../engine2d/DrawTask.h"
#include "../engine2d/DrawResource.h"
#include "../utils/BoundedBufferQueue.hpp"
#include "../utils/DrawResourceCollectorQueue.h"

#define RENDER_WORKER_THREAD_DEAD_MARK 0xdeaddead

class RenderWorkerThread {
public:
    RenderWorkerThread();
    void init(uint32_t workerId, BoundedBufferQueue<std::shared_ptr<DrawTask>> *bbqIn, DrawResourceCollectorQueue *drcqOut);

    void main(); // 主函数

    void start(); // 开始线程
    void join(); // 等待线程终止
private:
    /* 线程相关变量 */
    std::thread thread_;
    std::atomic_bool running_;

    uint32_t workerId_; // 每个线程有一个id，从0开始计数
    BoundedBufferQueue<std::shared_ptr<DrawTask>> *bbqIn_;
    DrawResourceCollectorQueue *drcqOut_;
};


#endif //PRF_RENDERWORKERTHREAD_H
