//
// Created by richardwu on 10/31/24.
//

#include "RenderWorkerThread.h"
#include <android/trace.h>

// TODO: 测试用
#include "../engine2d/rects/Rect.h"
#include "../engine2d/paint/Paint.h"
#include "../engine2d/Engine2D.h"

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

void RenderWorkerThread::init(uint32_t workerId, BoundedBufferQueue<std::shared_ptr<DrawTask>> *bbqIn, DrawResourceCollectorQueue *drcqOut) {
    workerId_ = workerId;
    bbqIn_ = bbqIn;
    drcqOut_ = drcqOut;
}

void RenderWorkerThread::main() {

    ATrace_beginSection((std::string("workerThread ") + std::to_string(workerId_)).c_str());
    while(running_.load() || bbqIn_->getSize() != 0) {
        // 获取数据，会阻塞
        std::shared_ptr<DrawTask> drawTask = bbqIn_->consume();

        // 程序退出时使用
        if(drawTask->getTaskId() == RENDER_WORKER_THREAD_DEAD_MARK) {
            break;
        }

        if (drawTask->getType() == RECTS_DRAWTASK) {
            ATrace_beginSection((std::string("rects_task") + std::to_string(drawTask->getTaskId())).c_str());
        } else if (drawTask->getType() == CIRCLES_DRAWTASK) {
            ATrace_beginSection((std::string("circles_task") + std::to_string(drawTask->getTaskId())).c_str());
        } else if (drawTask->getType() == IMAGE_DRAWTASK) {
            ATrace_beginSection((std::string("image_task") + std::to_string(drawTask->getTaskId()) + ": " + dynamic_cast<ImageDrawTask*>(drawTask.get())->image_.path_).c_str());
        } else {
            ATrace_beginSection((std::string("task") + std::to_string(drawTask->getTaskId())).c_str());
        }


        // 调用drawTask重写的draw函数。每个不同种类的drawTask内部调用Engine2D::drawXXX
        DrawResource drawResource = drawTask->draw();

        // 返回数据
        drawResource.taskId_ = drawTask->getTaskId();
        drcqOut_->produce(drawResource);

        ATrace_endSection();

    }
    ATrace_endSection();

}