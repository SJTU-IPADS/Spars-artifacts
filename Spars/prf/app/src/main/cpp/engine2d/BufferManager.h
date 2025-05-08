//
// Created by richardwu on 10/22/24.
//

#ifndef PRF_BUFFERMANAGER_H
#define PRF_BUFFERMANAGER_H

#include <vulkan_wrapper.h>

#include <map>
#include <list>
#include <mutex>
#include <string>

// 缓冲管理信息
struct VulkanBufferInfo {
    VkBuffer buffer_;
    VkDeviceMemory bufferMemory_;
    uint64_t size_; // 该buffer的大小，需为2的整数次幂
};

/*
 * 管理系统中所有的某种类型的VkBuffer
 * 暂时有两个实例，处理index buffer或vertex buffer, TODO: uniform buffer
 */
class BufferManager
{
public:
    BufferManager(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferUsageFlags usage); // 该BufferManager管理的VkBuffer类型
    ~BufferManager(); // 释放所有的VkBuffer和VkDeviceMemory
    void freeAllBuffers(uint32_t frameIndex); // 归还该帧使用的所有缓冲
    VulkanBufferInfo allocBuffer(uint32_t frameIndex, uint64_t size); // 为帧frameIndex申请一个大小至少为size的VkBuffer

    void dump(); // 以log的形式打印 for debug

private:
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkBufferUsageFlags usage_;

    std::mutex mutex_; // 保护下面两个list

    const uint64_t MIN_BUFFER_SIZE = 32L;
    std::map<uint64_t, std::list<VulkanBufferInfo>> freeBufferLists_; // 按照2的整数次幂管理所有free buffers
    std::map<uint32_t, std::list<VulkanBufferInfo>> usedBufferLists_; // 按照正在被哪一个轮转的帧使用，管理所有的used buffers

    uint64_t roundUpToPowerOfTwo(uint64_t size);
    bool mapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
    void createBuffer(VkDeviceSize size, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
};

#endif //PRF_BUFFERMANAGER_H
