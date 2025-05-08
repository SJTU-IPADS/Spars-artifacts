//
// Created by richardwu on 11/18/24.
//

#ifndef PRF_SAMPLERDESCRIPTORMANAGER_H
#define PRF_SAMPLERDESCRIPTORMANAGER_H

#include <vulkan_wrapper.h>
#include <shared_mutex>
#include <condition_variable>

#include "image/Image.h"

// 最多可以绘制的图片数量 TODO: 后续以LRU等策略管理
#define MAX_SAMPLER_DESCRIPTOR_COUNT 256

struct VulkanDescriptorSetInfo {
    bool valid_ = false; // 有些DrawResource可能不需要descriptor，RenderWorkerPool在插入时需要判断
    VkDescriptorSet descriptorSet_;
};

class SamplerDescriptorManager {
public:
    SamplerDescriptorManager(VkDevice device);
    ~SamplerDescriptorManager();

    void createAndInsertSamplerDescriptor(VkImageView imageView, VkDescriptorSetLayout descriptorSetLayout, VulkanImageInfo &vulkanImageInfo, VkDescriptorSet *descriptorSet); // 创建并插入VkDescriptorSet，创建的值通过descriptorSet参数返回

    /**
     * 查找是否有缓存的资源：
     *  若有：      返回该资源（通过参数）
     *  若没有：    若当前正有其他并发任务在创建该资源，则阻塞，在其创建完成后返回该资源
     *             若当前为第一个使用该未创建资源的任务，则直接返回false，并将该资源列为“准备中”（其他并发任务会阻塞）
     * @return 是否有缓存的资源
     */
    bool findSamplerDescriptor(VkImageView imageView, VkDescriptorSet *descriptorSet); // 返回是否找到

private:

    std::shared_mutex mutex_; // 保护下面的map
    std::condition_variable_any cv_; // 确保同时只有一个任务在创建资源

    std::unordered_map<VkImageView, VkDescriptorSet> descriptorSetMap_; // 此处的索引为VkImageView，决定对应的descriptor
    std::unordered_map<VkImageView, bool> preparingMap_; // 维护所有正在创建的Descriptor，避免多任务并发导致的重复创建

    VkDevice device_;
    VkDescriptorPool descriptorPool_;
};


#endif //PRF_SAMPLERDESCRIPTORMANAGER_H
