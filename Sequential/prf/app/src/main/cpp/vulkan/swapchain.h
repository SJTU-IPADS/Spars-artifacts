//
// Created by richardwu on 2024/10/19.
//

#ifndef PRF_SWAPCHAIN_H
#define PRF_SWAPCHAIN_H

#include <vulkan_wrapper.h>
#include "utils.h"
#include "../log.h"

#include <vector>

// 创建交换链
void getSwapChain(VkSurfaceKHR surface,
                     VkPhysicalDevice physicalDevice,
                     uint32_t queueFamilyIndex,
                     VkDevice device,
                     VulkanSwapchainInfo *swapchain) {

    memset(swapchain, 0, sizeof(VulkanSwapchainInfo));

    // **********************************************************
    // Get the surface capabilities because:
    //   - It contains the minimal and max length of the chain, we will need it
    //   - It's necessary to query the supported surface format (R8G8B8A8 for
    //   instance ...)
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &surfaceCapabilities);
    // Query the list of supported surface format and choose one we like
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                         &formatCount, nullptr);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                         &formatCount, formats);

    // 使用 VK_FORMAT_R8G8B8A8_UNORM
    uint32_t chosenFormat;
    for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
        if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break;
    }
    assert(chosenFormat < formatCount);

    swapchain->displaySize_ = surfaceCapabilities.currentExtent;
    swapchain->displayFormat_ = formats[chosenFormat].format;

    VkSurfaceCapabilitiesKHR surfaceCap;
    CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,
                                                      surface,&surfaceCap));
    assert(surfaceCap.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR); // 应该是考虑到安卓使用HWC合成

    // **********************************************************
    // Create a swap chain (here we choose the minimum available number of surface
    // in the chain)
    VkSwapchainCreateInfoKHR swapchainCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .surface = surface,
            .minImageCount = surfaceCapabilities.minImageCount,
            .imageFormat = formats[chosenFormat].format,
            .imageColorSpace = formats[chosenFormat].colorSpace,
            .imageExtent = surfaceCapabilities.currentExtent,
            .imageArrayLayers = 1, // 每个图像所包含的层次，非VR都是1
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // 对图像做什么操作：附上颜色。其他的还有后期处理等
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // 一张图像同时只能被一个队列族所有，必须要显示改变所有权，性能最佳
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queueFamilyIndex,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_FALSE,
            .oldSwapchain = VK_NULL_HANDLE,
    };
    CALL_VK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr,
                                 &swapchain->swapchain_));

    // 获取交换链中图像的数量（这里没有获取图像对象）
    CALL_VK(vkGetSwapchainImagesKHR(device, swapchain->swapchain_,
                                    &swapchain->swapchainLength_, nullptr));

    // 打印交换链中 的一些信息
    LOGI("swapChain:");
    LOGI("\tformat: %d, %d", formats[chosenFormat].format, formats[chosenFormat].colorSpace);
    LOGI("\tmode: VK_PRESENT_MODE_FIFO_KHR");
    LOGI("\textent: w=%d, h=%d", swapchain->displaySize_.width, swapchain->displaySize_.height);
    LOGI("\timageCount: %d", swapchain->swapchainLength_);
}

void DeleteSwapChain(VkDevice device, VulkanSwapchainInfo *swapchain) {
    for (int i = 0; i < swapchain->swapchainLength_; i++) {
        vkDestroyFramebuffer(device, swapchain->framebuffers_[i], nullptr);
        vkDestroyImageView(device, swapchain->displayViews_[i], nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain->swapchain_, nullptr);
}

#endif //PRF_SWAPCHAIN_H
