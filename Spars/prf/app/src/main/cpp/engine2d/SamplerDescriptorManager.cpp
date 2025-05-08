//
// Created by richardwu on 11/18/24.
//

#include "SamplerDescriptorManager.h"

#include <vulkan_wrapper.h>
#include <array>

/**
 * 创建描述符集
 */
static VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VulkanImageInfo &vulkanImageInfo)
{
    // 只为该图像创建一个描述符集
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet; // 描述符集对象

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }


//        // 配置描述符引用的缓冲对象
//        VkDescriptorBufferInfo bufferInfo = {};
//        bufferInfo.buffer = uniformBuffers[i];
//        bufferInfo.offset = 0;
//        bufferInfo.range = sizeof(UniformBufferObject);

    // 组合图像采样器
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vulkanImageInfo.textureImageView_;
    imageInfo.sampler = vulkanImageInfo.textureSampler_;

    // 用于更新描述符配置的vkUpdateDescriptorSets
    std::array<VkWriteDescriptorSet, 1> descriptorWrites = {}; // 原本是2

//        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrites[0].dstSet = descriptorSets[i];
//        descriptorWrites[0].dstBinding = 0;
//        descriptorWrites[0].dstArrayElement = 0;
//        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        descriptorWrites[0].descriptorCount = 1;
//        descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr); // 这样我们就可以在着色器中使用描述符了

    return descriptorSet;
}

void SamplerDescriptorManager::createAndInsertSamplerDescriptor(VkImageView imageView, VkDescriptorSetLayout descriptorSetLayout, VulkanImageInfo &vulkanImageInfo, VkDescriptorSet *descriptorSet) {
    std::unique_lock<std::shared_mutex> locker(mutex_);

    *descriptorSet = createDescriptorSet(device_, descriptorSetLayout, descriptorPool_, vulkanImageInfo);
    descriptorSetMap_[imageView] = *descriptorSet;

    // 资源创建完成，从准备列表移除
    preparingMap_.erase(imageView);
    cv_.notify_all(); // 此唤醒会导致所有任务唤醒，尽管有些任务在等待其他资源创建完成
}

bool SamplerDescriptorManager::findSamplerDescriptor(VkImageView imageView, VkDescriptorSet *descriptorSet) {
    {
        std::shared_lock<std::shared_mutex> locker(mutex_);
        auto iter = descriptorSetMap_.find(imageView);
        if (iter != descriptorSetMap_.end()) {
            *descriptorSet = iter->second;
            return true;
        }
    }
    {
        std::unique_lock<std::shared_mutex> locker(mutex_);
        // 未找到，判断是否有其他线程正在创建
        auto iter2 = preparingMap_.find(imageView);
        if (iter2 != preparingMap_.end()) { // 其他线程正在创建
            cv_.wait(locker, [this, &imageView] {
                return preparingMap_.find(imageView) == preparingMap_.end(); // 该资源已经不在准备列表中了
            });

            // 其他线程创建完毕了
            auto iter3 = descriptorSetMap_.find(imageView);
            if (iter3 != descriptorSetMap_.end()) {
                *descriptorSet = iter3->second;
                return true;
            }
            // 此处不应该走到
            throw std::runtime_error("SamplerDescriptorManager::findSamplerDescriptor error!");
            return false;
        } else {
            // 我们是第一个创建的
            preparingMap_[imageView] = true; // 将该资源加入准备列表，这个true不重要
            return false; // 需要我们后续创建
        }
    }
}

/**
* 创建描述符池
*/
static VkDescriptorPool createDescriptorPool(VkDevice device)
{
//    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
//    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
//    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // 组合图像采样器
//    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // 描述符池可以分配的描述符集
    std::array<VkDescriptorPoolSize, 1> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // 组合图像采样器
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_SAMPLER_DESCRIPTOR_COUNT);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_SAMPLER_DESCRIPTOR_COUNT);

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    return descriptorPool;
}

SamplerDescriptorManager::SamplerDescriptorManager(VkDevice device) {
    device_ = device;
    descriptorPool_ = createDescriptorPool(device);
}

SamplerDescriptorManager::~SamplerDescriptorManager() {
    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
}