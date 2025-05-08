//
// Created by richardwu on 10/21/24.
//

#ifndef PRF_COMMAND_BUFFERS_H
#define PRF_COMMAND_BUFFERS_H

#include <vulkan_wrapper.h>
#include "utils.h"

void getCommandBuffers(VkDevice device, uint32_t commandBufferCount, VkCommandPool commandPool, VulkanRenderInfo *render) {
    render->cmdBuffer_.resize(commandBufferCount);
    VkCommandBufferAllocateInfo cmdBufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = commandBufferCount,
    };
    CALL_VK(vkAllocateCommandBuffers(device, &cmdBufferCreateInfo, render->cmdBuffer_.data()));
}

#endif //PRF_COMMAND_BUFFERS_H
