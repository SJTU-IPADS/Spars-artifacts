//
// Created by richardwu on 10/21/24.
//

#ifndef PRF_COMMAND_POOL_H
#define PRF_COMMAND_POOL_H

#include <vulkan_wrapper.h>
#include "utils.h"

VkCommandPool getCommandPool(VkDevice device, uint32_t queueFamilyIndex) {
    // Create a pool of command buffers to allocate command buffer from
    VkCommandPoolCreateInfo cmdPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndex,
    };
    VkCommandPool commandPool;
    CALL_VK(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr,
                                &commandPool));
    return commandPool;
}

#endif //PRF_COMMAND_POOL_H
