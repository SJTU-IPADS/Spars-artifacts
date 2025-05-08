//
// Created by richardwu on 2024/10/19.
//

#ifndef PRF_SURFACE_H
#define PRF_SURFACE_H

#include <vulkan_wrapper.h>
#include "utils.h"

VkSurfaceKHR getSurface(VkInstance instance, ANativeWindow *platformWindow) {
    // 创建surface
    VkAndroidSurfaceCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = platformWindow};

    VkSurfaceKHR surface;
    CALL_VK(vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr,
                                      &surface));

    return surface;
}

#endif //PRF_SURFACE_H
