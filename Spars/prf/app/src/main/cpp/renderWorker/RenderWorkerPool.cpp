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
//    dtpIn_.setMaxSize(UINT32_MAX);
}

void RenderWorkerPool::init() {
    uint32_t workerId = 0;
    for(RenderWorkerThread *thread : threads_) {
        thread->init(workerId, &dtpIn_, &drcqOut_);
        workerId++;
    }
}

void RenderWorkerPool::start() {
    for(RenderWorkerThread *thread : threads_) {
        thread->start();
    }
}

void RenderWorkerPool::join() {
    dtpIn_.resetDummy(threads_.size());
    for(RenderWorkerThread *thread : threads_)
    {
        thread->join();
    }
}

void RenderWorkerPool::renderAll(VulkanDeviceInfo &deviceInfo, VulkanSwapchainInfo& swapchainInfo,
                                 VulkanRenderInfo &renderInfo, uint32_t frameIndex, DrawTaskList *drawTaskList) {

    // 重置收集
    drcqOut_.reset(drawTaskList);

    // 重置发送，并将任务任务放置好，最后通知工作线程开始工作。该步骤必须在重置收集之后
    dtpIn_.reset(drawTaskList);

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


    const uint32_t numTasks = drawTaskList->getTaskNum();
    for(uint32_t i = 0; i < numTasks; i++) {
        auto drawResource = drcqOut_.consume();

        //ATrace_beginSection(("Task" + std::to_string(drawResource->taskId_) + " collect").c_str());

        // 可以乱序
//        if(drawResource->taskId_ != i) {
//            LOGE("test error");
//        }

        // 绑定VkPipeline
        vkCmdBindPipeline(renderInfo.cmdBuffer_[frameIndex],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, drawResource->pipelineInfo_.pipeline_);

        // 绑定vertex buffer
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(renderInfo.cmdBuffer_[frameIndex], 0, 1,
                               &drawResource->vertexBufferInfo_.buffer_, &offset);

        // 绑定index buffer
        vkCmdBindIndexBuffer(renderInfo.cmdBuffer_[frameIndex],
                             drawResource->indexBufferInfo_.buffer_, 0, VK_INDEX_TYPE_UINT32);

        // 绑定描述符集（只有部分图元绘制用到）
        if (drawResource->descriptorSetInfo_.valid_) {
            vkCmdBindDescriptorSets(renderInfo.cmdBuffer_[frameIndex],
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    drawResource->pipelineInfo_.pipelineLayout_,
                                    0, 1, &drawResource->descriptorSetInfo_.descriptorSet_, 0,
                                    nullptr);
        }

        // 记录draw call
        vkCmdDrawIndexed(renderInfo.cmdBuffer_[frameIndex], drawResource->indexCount_, 1, 0, 0, 0);

        //ATrace_endSection();
    }

    vkCmdEndRenderPass(renderInfo.cmdBuffer_[frameIndex]);

    CALL_VK(vkEndCommandBuffer(renderInfo.cmdBuffer_[frameIndex]));

    ATrace_endSection(); // "RecordCmd"
}