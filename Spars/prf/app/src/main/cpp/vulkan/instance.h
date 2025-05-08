//
// Created by richardwu on 2024/10/18.
//

#ifndef PRF_INSTANCE_H
#define PRF_INSTANCE_H

#include <vulkan_wrapper.h>
#include "utils.h"
#include "../log.h"

#include <vector>

VkInstance getInstance() {

    // 应用信息
    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "prf-android",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_VERSION(1, 1, 0), // 使用vulkan 1.1
    };

    // 所需扩展
    std::vector<const char *> instance_extensions; // 实例扩展
    instance_extensions.push_back("VK_KHR_surface");
    instance_extensions.push_back("VK_KHR_android_surface");

    LOGI("instance extensions needed:");
    for (const auto &extension: instance_extensions) {
        LOGI("\t%s", extension);
    }

    // Create the Vulkan instance
    VkInstanceCreateInfo instanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount =
            static_cast<uint32_t>(instance_extensions.size()),
            .ppEnabledExtensionNames = instance_extensions.data(),
    };

    VkInstance instance;
    CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

    return  instance;
}

#endif //PRF_INSTANCE_H
