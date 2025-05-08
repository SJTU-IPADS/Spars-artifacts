//
// Created by richardwu on 10/29/24.
//

#ifndef PRF_PIPELINEMANAGER_H
#define PRF_PIPELINEMANAGER_H

#include <vulkan_wrapper.h>

#include <unordered_map>
#include <mutex>

// 所有渲染管线的种类，作为key
#define RECT_PIPELINE 1
#define CIRCLE_PIPELINE 2
#define IMAGE_PIPELINE 3
#define TEXT_PIPELINE 4
#define RRECT_PIPELINE 5

// 渲染管线信息
struct VulkanPipelineInfo {
    VkPipeline pipeline_;
    VkPipelineLayout pipelineLayout_;
    bool hasDescriptorSetLayout_ = false;
    VkDescriptorSetLayout descriptorSetLayout_;
};

/* 以哈希表的形式管理系统中所有的VkPipeline
 * TODO: 维护LRU,
 * TODO：是否需要将key变为shader */
class PipelineManager {
public:
    PipelineManager(VkDevice device);
    ~PipelineManager(); // 删除pipeline相关对象

    void insertPipeline(uint32_t key, VulkanPipelineInfo pipelineInfo);

    /**
     * 查找是否有缓存的资源：
     *  若有：      返回该资源（通过参数）
     *  若没有：    若当前正有其他并发任务在创建该资源，则阻塞，在其创建完成后返回该资源
     *             若当前为第一个使用该未创建资源的任务，则直接返回false，并将该资源列为“准备中”（其他并发任务会阻塞）
     * @return 是否有缓存的资源
     */
    bool findPipeline(uint32_t key, VulkanPipelineInfo *pipelineInfo); // 返回是否找到

private:
    VkDevice device_;

    std::mutex mutex_; // 保护下面的map
    std::condition_variable cv_; // 确保同时只有一个任务在创建资源
    std::unordered_map<uint32_t, VulkanPipelineInfo> pipelineMap_;
    std::unordered_map<uint32_t, bool> preparingMap_; // 维护所有正在创建的Pipeline，避免多任务并发导致的重复创建
};

#endif //PRF_PIPELINEMANAGER_H
