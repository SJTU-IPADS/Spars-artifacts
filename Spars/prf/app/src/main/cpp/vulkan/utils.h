//
// Created by richardwu on 2024/10/18.
//

#ifndef PRF_VULKAN_UTILS_H
#define PRF_VULKAN_UTILS_H

#include <vulkan_wrapper.h>
#include "../log.h"

#include <cassert>
#include <vector>

// Vulkan调用封装
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR,                            \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

// Vulkan设备信息
struct VulkanDeviceInfo {
    bool initialized_;

    VkInstance instance_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    uint32_t queueFamilyIndex_;

    VkSurfaceKHR surface_;
    VkQueue queue_;
};

// Vulkan交换链信息
struct VulkanSwapchainInfo {
    VkSwapchainKHR swapchain_;
    uint32_t swapchainLength_;

    VkExtent2D displaySize_;
    VkFormat displayFormat_;

    // array of frame buffers and views
    std::vector<VkImage> displayImages_;
    std::vector<VkImageView> displayViews_;
    std::vector<VkFramebuffer> framebuffers_;
};

// Vulkan RenderPass信息
struct VulkanRenderInfo {
    VkRenderPass renderPass_;
    VkCommandPool cmdPool_;
    std::vector<VkCommandBuffer> cmdBuffer_; // 每个帧缓冲有一个VkCommandBuffer（3或4）
    VkSemaphore imageAvailableSemaphore_;
    VkFence renderFinishedFence_;
    // VkPipelineCache pipelineCache_; // TODO: 暂时不使用VkPipelineCache
};

#endif //PRF_VULKAN_UTILS_H
