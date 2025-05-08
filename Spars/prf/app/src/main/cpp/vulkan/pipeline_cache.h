//
// Created by richardwu on 10/21/24.
//

#ifndef PRF_PIPELINE_CACHE_H
#define PRF_PIPELINE_CACHE_H

#include <vulkan_wrapper.h>
#include "utils.h"

VkPipelineCache getPipelineCache(VkDevice device) {
    VkPipelineCacheCreateInfo pipelineCacheInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,  // reserved, must be 0
            .initialDataSize = 0,
            .pInitialData = nullptr,
    };

    VkPipelineCache pipelineCache;
    CALL_VK(vkCreatePipelineCache(device, &pipelineCacheInfo, nullptr,
                                  &pipelineCache));
    return pipelineCache;
}

#endif //PRF_PIPELINE_CACHE_H
