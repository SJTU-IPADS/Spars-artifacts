//
// Created by richardwu on 2024/10/20.
//

#ifndef PRF_FRAME_BUFFERS_H
#define PRF_FRAME_BUFFERS_H

#include <vulkan_wrapper.h>
#include "utils.h"

// 依次创建Image、imageView、FrameBuffer
void getFrameBuffers(VkDevice device, VkRenderPass renderPass, VulkanSwapchainInfo *swapchain) {
    // query display attachment to swapchain
    uint32_t SwapchainImagesCount = swapchain->swapchainLength_;

    swapchain->displayImages_.resize(SwapchainImagesCount);

    CALL_VK(vkGetSwapchainImagesKHR(device, swapchain->swapchain_,
                                    &SwapchainImagesCount,
                                    swapchain->displayImages_.data()));

    // create image view for each swapchain image
    swapchain->displayViews_.resize(SwapchainImagesCount);
    for (uint32_t i = 0; i < SwapchainImagesCount; i++) {
        VkImageViewCreateInfo viewCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = swapchain->displayImages_[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D, // 二维纹理
                .format = swapchain->displayFormat_,
                .components =
                        {
                                // 使用默认的颜色通道映射（比如单色纹理可以将所有颜色通道映射到红色）
                                .r = VK_COMPONENT_SWIZZLE_R,
                                .g = VK_COMPONENT_SWIZZLE_G,
                                .b = VK_COMPONENT_SWIZZLE_B,
                                .a = VK_COMPONENT_SWIZZLE_A,
                        },
                .subresourceRange =
                        {
                                // subresourceRange指定图像用途和哪一部分可以被使用
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                        },
        };
        CALL_VK(vkCreateImageView(device, &viewCreateInfo, nullptr,
                                  &swapchain->displayViews_[i]));
    }
    // create a framebuffer from each swapchain image
    swapchain->framebuffers_.resize(swapchain->swapchainLength_);
    for (uint32_t i = 0; i < swapchain->swapchainLength_; i++) {
        VkImageView attachments[] = {
                swapchain->displayViews_[i]
        };
        VkFramebufferCreateInfo fbCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .renderPass = renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = swapchain->displaySize_.width,
                .height = swapchain->displaySize_.height,
                .layers = 1,
        };

        CALL_VK(vkCreateFramebuffer(device, &fbCreateInfo, nullptr,
                                    &swapchain->framebuffers_[i]));
    }
}

#endif //PRF_FRAME_BUFFERS_H
