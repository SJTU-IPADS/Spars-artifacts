//
// Created by richardwu on 2024/10/19.
//

#ifndef PRF_DEVICE_H
#define PRF_DEVICE_H

#include <vulkan_wrapper.h>
#include "utils.h"
#include "../log.h"

#include <vector>

VkDevice getDevice(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {

    // 所需设备扩展
    std::vector<const char *> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");

    LOGI("device extensions needed:");
    for (const auto &extension: device_extensions) {
        LOGI("\t%s", extension);
    }

    // Create a logical device (vulkan device)
    float priorities = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1, // 针对一个队列族我们所需的队列数量
            .pQueuePriorities = &priorities, // 必须显示地赋予队列优先级
    };

    VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
            .ppEnabledExtensionNames = device_extensions.data(),
            .pEnabledFeatures = nullptr,
    };

    VkDevice device;
    CALL_VK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr,
                           &device));
    return device;
}

#endif //PRF_DEVICE_H
