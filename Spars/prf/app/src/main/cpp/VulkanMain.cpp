// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VulkanMain.hpp"

#include "log.h"

// Vulkan相关
#include "vulkan/utils.h"
#include "vulkan/instance.h"
#include "vulkan/surface.h"
#include "vulkan/physical_device.h"
#include "vulkan/queue_family_index.h"
#include "vulkan/device.h"
#include "vulkan/queue.h"
#include "vulkan/swapchain.h"
#include "vulkan/render_pass.h"
#include "vulkan/frame_buffers.h"
#include "vulkan/command_pool.h"
#include "vulkan/command_buffers.h"
#include "vulkan/sync_objects.h"
#include "vulkan/pipeline_cache.h"

#include "engine2d/image_layout.h"
#include "engine2d/BufferManager.h"
#include "engine2d/PipelineManager.h"
#include "engine2d/ImageManager.h"
#include "engine2d/Engine2D.h"

#include "renderTree/RenderNode.h"
#include "renderTree/AnimationsList.h"

#include "drawTaskContainer/DrawTaskList.h"

#include "renderWorker/RenderWorkerPool.h"

#include "treeParser/TreeParser.h"
#include "config.h"

#include <vulkan_wrapper.h>

#include <cassert>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <android/trace.h>

VulkanDeviceInfo deviceInfo;
VulkanSwapchainInfo swapchainInfo;
VulkanRenderInfo renderInfo;

RenderWorkerPool renderWorkerPool;

RenderNode *rootNode; // 所需绘制内容（渲染树）的根节点
AnimationsList animationsList; // 所有动画列表
uint64_t frameIndex = 0; // TODO: 移入vsync


RenderNode *testRenderTree() {
    /* 根节点 */
    RenderNode *rootNode = new RenderNode(nullptr, 0, 0, 0, 1000, 2000);

    for(int i = 0; i < 700; i++) {
        /* 节点 */
        RenderNode *rectNode = new RenderNode(rootNode, i+1, rand()%900, rand()%1900, 300, 100);
        rootNode->addChild(rectNode);

        // 选取一个随机图元
        int g = rand() % 5;

        if (g == 0) {
            Paint paint;
            paint.setColor(rand());
            RRect rrect = RRect::MakeXYWHR(0, 0, 300, 100, 20);
            auto rrectCmd = std::make_shared<RRectDrawCmd>(paint, rrect);
            rectNode->addDrawCmd(rrectCmd);
        } else if (g == 1) {
            Paint paint;
            paint.setColor(rand());
            Text text = Text::MakeText(30, 20, 50, "AAABBB", "/system/fonts/DroidSans.ttf");
            auto textCmd = std::make_shared<TextDrawCmd>(paint, text);
            rectNode->addDrawCmd(textCmd);
        } else if (g == 2) {
            Paint paint;
            Image image = Image::MakeImage(Rect::MakeXYWH(50, 0, 100, 100), "Texture/icon1.png");
            auto imageCmd = std::make_shared<ImageDrawCmd>(paint, image);
            rectNode->addDrawCmd(imageCmd);
        } else if (g == 3) {
            Paint paint;
            paint.setColor(rand());
            Rect rect = Rect::MakeXYWH(0, 0, 300, 100);
            auto rectCmd = std::make_shared<RectDrawCmd>(paint, rect);
            rectNode->addDrawCmd(rectCmd);
        } else if (g == 4) {
            Paint paint;
            paint.setColor(rand());
            Circle circle = Circle::MakeXYR(50, 50, 50);
            auto circleCmd = std::make_shared<CircleDrawCmd>(paint, circle);
            rectNode->addDrawCmd(circleCmd);
        }

    }
    return rootNode;
}

/*
 * setImageLayout():
 *    Helper function to transition color buffer layout
 */
void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                    VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                    VkPipelineStageFlags srcStages,
                    VkPipelineStageFlags destStages);

// InitVulkan: Vulkan状态的初始化
//   Initialize Vulkan Context when android application window is created
//   upon return, vulkan is ready to draw frames
bool InitVulkan(android_app *app) {

    // 获取libvulkan.so中含有的vulkan函数
    if (!InitVulkan()) {
        LOGE("Vulkan is unavailable, install vulkan and re-start");
        return false;
    }

    // 依次创建vulkan全局数据结构
    deviceInfo.instance_ = getInstance();
    deviceInfo.surface_ = getSurface(deviceInfo.instance_, app->window);
    deviceInfo.physicalDevice_ = getPhysicalDevice(deviceInfo.instance_, deviceInfo.surface_);
    deviceInfo.queueFamilyIndex_ = getQueueFamilyIndex(deviceInfo.physicalDevice_);
    deviceInfo.device_ = getDevice(deviceInfo.physicalDevice_, deviceInfo.queueFamilyIndex_);
    deviceInfo.queue_ = getQueue(deviceInfo.device_, deviceInfo.queueFamilyIndex_);

    // 创建交换链
    getSwapChain(deviceInfo.surface_, deviceInfo.physicalDevice_, deviceInfo.queueFamilyIndex_, deviceInfo.device_, &swapchainInfo);

    // 创建render pass
    renderInfo.renderPass_ = getRenderPass(deviceInfo.device_, swapchainInfo.displayFormat_);

    // 依次创建Image、imageView、FrameBuffer
    getFrameBuffers(deviceInfo.device_, renderInfo.renderPass_, &swapchainInfo);

    // 创建指令池
    renderInfo.cmdPool_ = getCommandPool(deviceInfo.device_, deviceInfo.queueFamilyIndex_);

    // 创建指令缓冲（为帧缓冲中的每一帧）
    getCommandBuffers(deviceInfo.device_, swapchainInfo.swapchainLength_, renderInfo.cmdPool_, &renderInfo);

    // 创建同步原语
    getImageAvailableSemaphore(deviceInfo.device_, &renderInfo);
    getRenderFinishedFence(deviceInfo.device_, &renderInfo);

    // 创建管线缓存
    // renderInfo.pipelineCache_ = getPipelineCache(deviceInfo.device_);


// ============================ 以下为2d引擎资源管理 ============================

    // 初始化2D引擎绘制接口类
    Engine2D::init(app, &deviceInfo, &swapchainInfo, &renderInfo);


// ============================ 以下为渲染线程管理 ==============================
    renderWorkerPool.init();
    renderWorkerPool.start();

// ============================ 以下为所需绘制内容 ==============================
    TreeParser treeParser;
    int32_t width, height;
    rootNode = treeParser.parse(app, RS_TREE_PATH, width, height, &animationsList);
//    rootNode = testRenderTree();
//    LOGI("%s", rootNode->dumpTree().c_str());

    deviceInfo.initialized_ = true;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);          // 清空所有核心的标志位
    CPU_SET(MAINTHREAD_CORE, &cpuset);  // 设置核心 ID，例如将线程绑定到核心 1

    int result = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);  // 设置亲和性
    if (result != 0) {
        LOGE("fail to set affinity %d!", MAINTHREAD_CORE);
    } else {
        LOGI("binds to core %d", MAINTHREAD_CORE);
    }

    return true;
}

// IsVulkanReady():
//    native app poll to see if we are ready to draw...
bool IsVulkanReady() {
    return deviceInfo.initialized_;
}

void DeleteVulkan() {
    renderWorkerPool.join();

    vkDestroySemaphore(deviceInfo.device_, renderInfo.imageAvailableSemaphore_, nullptr);
    vkDestroyFence(deviceInfo.device_, renderInfo.renderFinishedFence_, nullptr);

    vkFreeCommandBuffers(deviceInfo.device_, renderInfo.cmdPool_, renderInfo.cmdBuffer_.size(),
                         renderInfo.cmdBuffer_.data());
    renderInfo.cmdBuffer_.clear();

    vkDestroyCommandPool(deviceInfo.device_, renderInfo.cmdPool_, nullptr);
    vkDestroyRenderPass(deviceInfo.device_, renderInfo.renderPass_, nullptr);
    DeleteSwapChain(deviceInfo.device_, &swapchainInfo);

    Engine2D::del(); // 删除Engine2D维护的BufferManager和PipelineManager和ImageManager

    vkDestroyDevice(deviceInfo.device_, nullptr);
    vkDestroyInstance(deviceInfo.instance_, nullptr);

    deviceInfo.initialized_ = false;
}

// Draw one frame
bool VulkanDrawFrame(android_app *app) {

    ATrace_beginSection("MyVulkanDrawFrame");

    // 获取图片index
    uint32_t nextIndex;
    // Get the framebuffer index we should draw in
    CALL_VK(vkAcquireNextImageKHR(deviceInfo.device_, swapchainInfo.swapchain_,
                                  UINT64_MAX, renderInfo.imageAvailableSemaphore_, VK_NULL_HANDLE,
                                  &nextIndex));
    CALL_VK(vkResetFences(deviceInfo.device_, 1, &renderInfo.renderFinishedFence_));

    ATrace_beginSection("EndToEndDuration");
    // 动画
    ATrace_beginSection("animate");
    VSyncInfo vSyncInfo = { // TODO: 后续接入vsync
            .frameIndex = frameIndex++,
            .vsyncTS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() // now
    };

    if (animationsList.animateAll(vSyncInfo)) { // 执行动画
       animationsList.updateTree(rootNode);  // 迭代整棵树修改子节点
    }

    ATrace_endSection();


    // 生成DrawTaskContainer数据结构
    ATrace_beginSection("generateDrawTask");
    DrawTaskList drawTaskList;
    drawTaskList.generateFromRenderTree(rootNode);
    ATrace_endSection();

    // 填写绘制命令
    // 首先，重置该帧在上次轮转时使用的资源
    vkResetCommandBuffer(renderInfo.cmdBuffer_[nextIndex], 0);
    Engine2D::resetFrame(nextIndex);

    // 渲染（生成VkCommandBuffer）
    renderWorkerPool.renderAll(deviceInfo, swapchainInfo, renderInfo, nextIndex, &drawTaskList);

    // 提交指令
    VkPipelineStageFlags waitStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderInfo.imageAvailableSemaphore_,
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &renderInfo.cmdBuffer_[nextIndex],
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr};
    CALL_VK(vkQueueSubmit(deviceInfo.queue_, 1, &submit_info, renderInfo.renderFinishedFence_));
    ATrace_endSection();

    // Wait timeout set to 1 second
    CALL_VK(vkWaitForFences(deviceInfo.device_, 1, &renderInfo.renderFinishedFence_, VK_TRUE, 1000000000));


    // 递交显示
    ATrace_beginSection("SendPresent");
    VkResult result;
    VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &swapchainInfo.swapchain_,
            .pImageIndices = &nextIndex,
            .pResults = &result,
    };
    vkQueuePresentKHR(deviceInfo.queue_, &presentInfo);
    ATrace_endSection();

    ATrace_endSection();
    return true;
}
