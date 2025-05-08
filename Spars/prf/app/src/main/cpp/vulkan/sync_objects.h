//
// Created by richardwu on 10/21/24.
//

#ifndef PRF_SYNC_OBJECTS_H
#define PRF_SYNC_OBJECTS_H

#include <vulkan_wrapper.h>
#include "utils.h"

void getImageAvailableSemaphore(VkDevice device, VulkanRenderInfo *render) {
    VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    CALL_VK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                              &render->imageAvailableSemaphore_));
}

void getRenderFinishedFence(VkDevice device, VulkanRenderInfo *render) {
    VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    CALL_VK(vkCreateFence(device, &fenceCreateInfo, nullptr, &render->renderFinishedFence_));

}

#endif //PRF_SYNC_OBJECTS_H
