//
// Created by richardwu on 10/29/24.
//

#include "PipelineManager.h"

PipelineManager::PipelineManager(VkDevice device) {
    device_ = device;
}

PipelineManager::~PipelineManager() {
    std::unique_lock<std::mutex> locker(mutex_);
    for(auto iter = pipelineMap_.begin(); iter != pipelineMap_.end(); iter++) {
        VulkanPipelineInfo pipelineInfo = iter->second;

        if (pipelineInfo.hasDescriptorSetLayout_) {
            vkDestroyDescriptorSetLayout(device_, pipelineInfo.descriptorSetLayout_, nullptr);
        }
        vkDestroyPipeline(device_, pipelineInfo.pipeline_, nullptr);
        vkDestroyPipelineLayout(device_, pipelineInfo.pipelineLayout_, nullptr);
    }
}

void PipelineManager::insertPipeline(uint32_t key, VulkanPipelineInfo pipelineInfo) {
    std::unique_lock<std::mutex> locker(mutex_);
    pipelineMap_[key] = pipelineInfo;

    // 资源创建完成，从准备列表移除
    preparingMap_.erase(key);
    cv_.notify_all(); // 此唤醒会导致所有任务唤醒，尽管有些任务在等待其他资源创建完成
}

bool PipelineManager::findPipeline(uint32_t key, VulkanPipelineInfo *pipelineInfo) {
    std::unique_lock<std::mutex> locker(mutex_);
    auto iter = pipelineMap_.find(key);
    if (iter != pipelineMap_.end()) {
        *pipelineInfo = iter->second;
        return true;
    } else {
        // 未找到，判断是否有其他线程正在创建
        auto iter2 = preparingMap_.find(key);
        if (iter2 != preparingMap_.end()) { // 其他线程正在创建
            cv_.wait(locker, [this, &key] {
                return preparingMap_.find(key) == preparingMap_.end(); // 该资源已经不在准备列表中了
            });

            // 其他线程创建完毕了
            auto iter3 = pipelineMap_.find(key);
            if (iter3 != pipelineMap_.end()) {
                *pipelineInfo = iter3->second;
                return true;
            }
            // 此处不应该走到
            throw std::runtime_error("PipelineManager::findPipeline error!");
            return false;
        } else {
            // 我们是第一个创建的
            preparingMap_[key] = true; // 将该资源加入准备列表，这个true不重要
            return false; // 需要我们后续创建
        }
    }
}