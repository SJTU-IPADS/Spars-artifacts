//
// Created by richardwu on 10/31/24.
//

#ifndef PRF_RENDERWORKERPOOL_H
#define PRF_RENDERWORKERPOOL_H

#include <vector>
#include <memory>

#include "RenderWorkerThread.h"
#include "../engine2d/DrawTask.h"
#include "../engine2d/DrawResource.h"
#include "../drawTaskContainer/DrawTaskList.h"
#include "../utils/BoundedBufferQueue.hpp"
#include "../utils/DrawResourceCollectorQueue.h"
#include "../vulkan/utils.h"

#include "../config.h"

/* 渲染工作线程池 */
class RenderWorkerPool {
private:
    std::vector<RenderWorkerThread*> threads_; // 所有工作线程

    // 所有线程使用的数据结构
    BoundedBufferQueue<std::shared_ptr<DrawTask>> bbqIn_;
    DrawResourceCollectorQueue drcqOut_;

public:
    RenderWorkerPool();

    void init();
    void start();
    void join();
    void renderAll(VulkanDeviceInfo &deviceInfo, VulkanSwapchainInfo& swapchainInfo,
                                     VulkanRenderInfo &renderInfo, uint32_t frameIndex, DrawTaskList *drawTaskList);

};


#endif //PRF_RENDERWORKERPOOL_H
