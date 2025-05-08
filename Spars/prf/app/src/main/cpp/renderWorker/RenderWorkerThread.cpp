//
// Created by richardwu on 10/31/24.
//

#include "RenderWorkerThread.h"
#include <android/trace.h>

#include "../engine2d/rects/Rect.h"
#include "../engine2d/paint/Paint.h"
#include "../engine2d/Engine2D.h"

#include <pthread.h>
#include <sched.h>
#include <stdio.h>


int affinityArray[9] = {4, 6, 5, 7, 8, 0, 1, 2, 3};

// bind thread to specific cores
void set_thread_affinity(uint32_t worker_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(affinityArray[worker_id], &cpuset);

    int result = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);  // set affinity
    if (result != 0) {
        LOGE("fail to set affinity %d!", affinityArray[worker_id]);
    } else {
        LOGI("worker %u binds to core %d", worker_id, affinityArray[worker_id]);
    }
}

RenderWorkerThread::RenderWorkerThread() {
    std::atomic_init(&running_, false);
}

void RenderWorkerThread::start() {
    // 运行该线程
    running_.store(true);
    thread_ = std::thread(std::bind(&RenderWorkerThread::main, this));
}

void RenderWorkerThread::join() {
    running_.store(false);
    thread_.join();
}

void RenderWorkerThread::init(uint32_t workerId, DrawTaskPool *dtpIn, DrawResourceCollectorQueue *drcqOut) {
    workerId_ = workerId;
    dtpIn_ = dtpIn;
    drcqOut_ = drcqOut;
}

void RenderWorkerThread::main() {
    if (affinityArray[workerId_] != -1) {
        set_thread_affinity(workerId_);
    }

    ATrace_beginSection((std::string("workerThread ") + std::to_string(workerId_)).c_str());
    while(running_.load()) {
        // 获取数据，当获取不到时会阻塞
        DrawTask *drawTask = dtpIn_->getNextDrawTask();

        // 程序退出时使用
        if(drawTask->getTaskId() == RENDER_WORKER_THREAD_DEAD_MARK) {
            delete drawTask;
            break;
        }

        switch(drawTask->getType()) {
            case RECTS_DRAWTASK:
                ATrace_beginSection((std::string("rects_task") + std::to_string(drawTask->getTaskId())).c_str());
                break;
            case CIRCLES_DRAWTASK:
                ATrace_beginSection((std::string("circles_task") + std::to_string(drawTask->getTaskId())).c_str());
                break;
            case IMAGE_DRAWTASK:
                ATrace_beginSection((std::string("image_task") + std::to_string(drawTask->getTaskId()) + ": " + dynamic_cast<ImageDrawTask*>(drawTask)->image_.path_).c_str());
                break;
            case TEXTS_DRAWTASK:
                ATrace_beginSection((std::string("texts_task") + std::to_string(drawTask->getTaskId())).c_str());
                break;
            case RRECTS_DRAWTASK:
                ATrace_beginSection((std::string("rrects_task") + std::to_string(drawTask->getTaskId())).c_str());
                break;
            default:
                ATrace_beginSection((std::string("task") + std::to_string(drawTask->getTaskId())).c_str());
        }

        // 调用drawTask重写的draw函数。每个不同种类的drawTask内部调用Engine2D::drawXXX
        std::shared_ptr<DrawResource> drawResource (new DrawResource(drawTask->draw()));

        // 返回数据
        drawResource->taskId_ = drawTask->getTaskId();
        drcqOut_->produce(drawResource);

        ATrace_endSection();

    }
    ATrace_endSection();

}