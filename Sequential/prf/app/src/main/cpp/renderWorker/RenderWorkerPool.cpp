//
// Created by richardwu on 10/31/24.
//

#include <android/trace.h>

#include "RenderWorkerPool.h"
#include "../log.h"

RenderWorkerPool::RenderWorkerPool() {
    for(int i = 0; i < RENDER_THREAD_COUNT; i++) {
        threads_.push_back(new RenderWorkerThread());
    }
    bbqIn_.setMaxSize(UINT32_MAX);
}

void RenderWorkerPool::init() {
    uint32_t workerId = 0;
    for(RenderWorkerThread *thread : threads_) {
        thread->init(workerId, &bbqIn_, &drcqOut_);
        workerId++;
    }
}

void RenderWorkerPool::start() {
    for(RenderWorkerThread *thread : threads_) {
        thread->start();
    }
}

void RenderWorkerPool::join() {
    // 发线程数量的 RENDER_WORKER_THREAD_DEAD_MARK，每个线程收到一次后便终结，因此每个线程只会收到一次
    for (int i = 0; i < threads_.size(); i++)
    {
        // a dummy draw task
        std::vector<Rect> rects;
        std::vector<Paint> paints;
        Rect boundingBox = Rect::MakeXYWH(0, 0, 0, 0);
        std::shared_ptr<RectsDrawTask> drawTask(new RectsDrawTask(RENDER_WORKER_THREAD_DEAD_MARK, boundingBox, rects, paints));

        bbqIn_.produce(std::move(drawTask));
    }
    for(RenderWorkerThread *thread : threads_)
    {
        thread->join();
    }
}

void RenderWorkerPool::renderAll(VulkanDeviceInfo &deviceInfo, VulkanSwapchainInfo& swapchainInfo,
                                 VulkanRenderInfo &renderInfo, uint32_t frameIndex, DrawTaskList *drawTaskList) {

    // 指令录制与收集完成的任务
    ATrace_beginSection("RecordCmd");
    // We create and declare the "beginning" our command buffer
    VkCommandBufferBeginInfo cmdBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
    };
    CALL_VK(vkBeginCommandBuffer(renderInfo.cmdBuffer_[frameIndex],
                                 &cmdBufferBeginInfo));
//    // transition the display image to color attachment layout // TODO: 这里格式转换的必要性？
//    setImageLayout(renderInfo.cmdBuffer_[nextIndex],
//                   swapchainInfo.displayImages_[nextIndex],
//                   VK_IMAGE_LAYOUT_UNDEFINED,
//                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
//                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Now we start a renderPass. Any draw command has to be recorded in a
    // renderPass
    VkClearValue clearVals = {{{1.0f, 1.0f, 1.0f, 0.0f}}};
    VkRenderPassBeginInfo renderPassBeginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = renderInfo.renderPass_,
            .framebuffer = swapchainInfo.framebuffers_[frameIndex],
            .renderArea = {.offset {.x = 0, .y = 0,},
                    .extent = swapchainInfo.displaySize_},
            .clearValueCount = 1,
            .pClearValues = &clearVals};
    vkCmdBeginRenderPass(renderInfo.cmdBuffer_[frameIndex], &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);


    for(uint32_t i = 0; i < drawTaskList->getTaskNum(); i++) {

        // 程序退出时使用
        auto drawTask = drawTaskList->getDrawTask(i);

        switch(drawTask->getType()) {
            case RECTS_DRAWTASK:
                ATrace_beginSection((std::string("rects_task") + std::to_string(drawTask->getTaskId())).c_str());
                break;
            case CIRCLES_DRAWTASK:
                ATrace_beginSection((std::string("circles_task") + std::to_string(drawTask->getTaskId())).c_str());
                break;
            case IMAGE_DRAWTASK:
                ATrace_beginSection((std::string("image_task") + std::to_string(drawTask->getTaskId()) + ": " + dynamic_cast<ImageDrawTask*>(drawTask.get())->image_.path_).c_str());
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
        DrawResource drawResource = drawTask->draw();

        // 绑定VkPipeline
        vkCmdBindPipeline(renderInfo.cmdBuffer_[frameIndex],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, drawResource.pipelineInfo_.pipeline_);

        // 绑定vertex buffer
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(renderInfo.cmdBuffer_[frameIndex], 0, 1,
                               &drawResource.vertexBufferInfo_.buffer_, &offset);

        // 绑定index buffer
        vkCmdBindIndexBuffer(renderInfo.cmdBuffer_[frameIndex],
                             drawResource.indexBufferInfo_.buffer_, 0, VK_INDEX_TYPE_UINT32);

        // 绑定描述符集（只有部分图元绘制用到）
        if (drawResource.descriptorSetInfo_.valid_) {
            vkCmdBindDescriptorSets(renderInfo.cmdBuffer_[frameIndex],
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    drawResource.pipelineInfo_.pipelineLayout_,
                                    0, 1, &drawResource.descriptorSetInfo_.descriptorSet_, 0,
                                    nullptr);
        }

        // 记录draw call
        vkCmdDrawIndexed(renderInfo.cmdBuffer_[frameIndex], drawResource.indexCount_, 1, 0, 0, 0);

        ATrace_endSection();
    }

    vkCmdEndRenderPass(renderInfo.cmdBuffer_[frameIndex]);

    CALL_VK(vkEndCommandBuffer(renderInfo.cmdBuffer_[frameIndex]));

    ATrace_endSection(); // "RecordCmd"
}