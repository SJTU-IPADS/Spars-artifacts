//
// Created by richardwu on 2024/10/19.
//

#ifndef PRF_PHYSICAL_DEVICE_H
#define PRF_PHYSICAL_DEVICE_H

#include <vulkan_wrapper.h>
#include "utils.h"

VkPhysicalDevice getPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting
    // graphics/compute/present
    // for this sample, we use the very first GPU device found on the system
    uint32_t gpuCount = 0;
    CALL_VK(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    VkPhysicalDevice tmpGpus[gpuCount];
    CALL_VK(vkEnumeratePhysicalDevices(instance, &gpuCount, tmpGpus));

    // 选择第一个设备，打印一些设备信息
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties( tmpGpus[0], &properties );
    LOGI("physical device: [%d] %s", properties.deviceID, properties.deviceName);

    return tmpGpus[0];
}

#endif //PRF_PHYSICAL_DEVICE_H
