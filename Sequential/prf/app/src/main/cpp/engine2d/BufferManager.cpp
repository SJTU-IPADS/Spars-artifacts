//
// Created by richardwu on 10/22/24.
//

#include "BufferManager.h"
#include "../vulkan/utils.h"

BufferManager::BufferManager(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags usage) {
    device_ = device;
    physicalDevice_ = physicalDevice;
    usage_ = usage;
}

BufferManager::~BufferManager() {
    std::unique_lock<std::mutex> locker(mutex_);
    // 释放所有的VkBuffer和VkDeviceMemory
    for(auto iter = freeBufferLists_.begin(); iter != freeBufferLists_.end(); iter++) {
        while (!iter->second.empty()) {
            // 取出
            VulkanBufferInfo bufferInfo = iter->second.front();
            iter->second.pop_front();

            // 删除
            vkDestroyBuffer(device_, bufferInfo.buffer_, nullptr);
            vkFreeMemory(device_, bufferInfo.bufferMemory_, nullptr);
        }
    }

    for(auto iter = usedBufferLists_.begin(); iter != usedBufferLists_.end(); iter++) {
        while (!iter->second.empty()) {
            // 取出
            VulkanBufferInfo bufferInfo = iter->second.front();
            iter->second.pop_front();

            // 删除
            vkDestroyBuffer(device_, bufferInfo.buffer_, nullptr);
            vkFreeMemory(device_, bufferInfo.bufferMemory_, nullptr);
        }
    }
}

void BufferManager::freeAllBuffers(uint32_t frameIndex) {
    std::unique_lock<std::mutex> locker(mutex_);
    std::list<VulkanBufferInfo>& usedBufferList = usedBufferLists_[frameIndex];

    // 将所有VkBuffer还至freeBufferLists_
    while(!usedBufferList.empty()) {
        // 取出
        VulkanBufferInfo bufferInfo = usedBufferList.front();
        usedBufferList.pop_front();

        // 加入
        freeBufferLists_[bufferInfo.size_].push_back(bufferInfo);
    }

}

VulkanBufferInfo BufferManager::allocBuffer(uint32_t frameIndex, uint64_t size) {
    std::unique_lock<std::mutex> locker(mutex_);
    size = roundUpToPowerOfTwo(size);
    std::list<VulkanBufferInfo>& freeBufferList = freeBufferLists_[size];

    // 已有空闲已分配VkBuffer
    if(!freeBufferList.empty()) {
        // 取出
        VulkanBufferInfo bufferInfo = freeBufferList.front();
        freeBufferList.pop_front();

        // 加入
        usedBufferLists_[frameIndex].push_back(bufferInfo);
        return bufferInfo;
    }
    // 创建新的VkBuffer
    else {

        // 创建
        VulkanBufferInfo bufferInfo;
        createBuffer(size, bufferInfo.buffer_, bufferInfo.bufferMemory_);
        bufferInfo.size_ = size;

        // 加入
        usedBufferLists_[frameIndex].push_back(bufferInfo);
        return bufferInfo;
    }
}

uint64_t BufferManager::roundUpToPowerOfTwo(uint64_t size) {
    if (size <= MIN_BUFFER_SIZE) return MIN_BUFFER_SIZE;
    size_t power = MIN_BUFFER_SIZE;
    while (power < size) {
        power *= 2;
    }
    return power;
}

/* 创建VkBuffer的一系辅助函数 */
bool BufferManager::mapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    // Search mem types to find first index with those properties
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) ==
                requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

/**
* 创建缓冲的辅助函数
* 可以使用不同的大小、usage、properties
*/
void BufferManager::createBuffer(VkDeviceSize size, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage_;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CALL_VK(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer));
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    mapMemoryTypeToIndex(memRequirements.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &allocInfo.memoryTypeIndex);

    CALL_VK(vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

void BufferManager::dump() {
    std::unique_lock<std::mutex> locker(mutex_);

    std::string usageStr;
    if (usage_ == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        usageStr = "vertex";
    } else if (usage_ == VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        usageStr = "index";
    } else {
        usageStr = "unknown";
    }
    LOGI("%s buffer manager:", usageStr.c_str());

    LOGI("\tfreeBufferLists_:");
    for(auto iter = freeBufferLists_.begin(); iter != freeBufferLists_.end(); iter++) {
        LOGI("\t\t[%d] size %d", iter->first, iter->second.size());
    }

    LOGI("\tusedBufferLists_:");
    for(auto iter = usedBufferLists_.begin(); iter != usedBufferLists_.end(); iter++) {
        LOGI("\t\t[%d] size %d", iter->first, iter->second.size());
    }
}