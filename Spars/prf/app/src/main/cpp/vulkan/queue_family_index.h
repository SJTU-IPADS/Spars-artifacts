//
// Created by richardwu on 2024/10/19.
//

#ifndef PRF_QUEUE_FAMILY_INDEX_H
#define PRF_QUEUE_FAMILY_INDEX_H

#include <vulkan_wrapper.h>
#include "utils.h"

#include <vector>
#include <cassert>

uint32_t getQueueFamilyIndex(VkPhysicalDevice physicalDevice) {
    // Find a GFX queue family
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             nullptr);
    assert(queueFamilyCount); // 至少需要一个队列族
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             queueFamilyProperties.data());

    // 找到一个支持graphics的队列族
    uint32_t queueFamilyIndex;
    for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
         queueFamilyIndex++) {
        if (queueFamilyProperties[queueFamilyIndex].queueFlags &
            VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    assert(queueFamilyIndex < queueFamilyCount); // 必须要找到
    return queueFamilyIndex;
}

#endif //PRF_QUEUE_FAMILY_INDEX_H
