//
// Created by richardwu on 2024/10/19.
//

#ifndef PRF_QUEUE_H
#define PRF_QUEUE_H

#include <vulkan_wrapper.h>

VkQueue getQueue(VkDevice device, uint32_t queueFamilyIndex) {
    VkQueue queue;
    //赋值队列句柄，参数依次是逻辑设备对象，队列族索引，队列索引
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    return queue;
}

#endif //PRF_QUEUE_H
