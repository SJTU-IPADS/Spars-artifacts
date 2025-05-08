//
// Created by richardwu on 11/18/24.
//

#ifndef PRF_IMAGEMANAGER_H
#define PRF_IMAGEMANAGER_H

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan_wrapper.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <condition_variable>

#include "image/Image.h"

class Image;

struct ImageHash {
    std::size_t operator()(const Image& obj) const;
};

// 图片（纹理）管理信息
struct VulkanImageInfo {
    VkImage textureImage_;              // 纹理图像
    VkDeviceMemory textureImageMemory_; // 纹理图像显存
    VkImageView textureImageView_;      // 纹理图像视图
    VkSampler textureSampler_;          // 采样器 // TODO: 采样器与图片本身解耦
};

/*
 * 管理所有的纹理VulkanImageInfo
 * TODO: 采用基于LRU的上限策略，最多维护 XX MB的纹理
 * 同时也负责管理所有的VkSampler
 */
class ImageManager {
public:
    ImageManager(android_app *app, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~ImageManager(); // 删除所有VkImage

    void createAndInsertImageInfo(Image &image, VulkanImageInfo *imageInfo); // 创建并插入VulkanImageInfo，创建的值通过imageInfo参数返回

    /**
     * 查找是否有缓存的资源：
     *  若有：      返回该资源（通过参数）
     *  若没有：    若当前正有其他并发任务在创建该资源，则阻塞，在其创建完成后返回该资源
     *             若当前为第一个使用该未创建资源的任务，则直接返回false，并将该资源列为“准备中”（其他并发任务会阻塞）
     * @return 是否有缓存的资源
     */
    bool findImageInfo(Image &image, VulkanImageInfo *imageInfo); // 返回是否找到

private:

    std::shared_mutex mutex_; // 保护下面的map
    std::condition_variable_any cv_; // 确保同时只有一个任务在创建资源
    std::unordered_map<Image, VulkanImageInfo, ImageHash> imageMap_;
    std::unordered_map<Image, bool, ImageHash> preparingMap_; // 维护所有正在创建的Image，避免多任务并发导致的重复创建

    android_app *app_;
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkCommandPool commandPool_;
    VkQueue graphicsQueue_;


    // 创建一张图片所有的内容
    VulkanImageInfo createTextureImageInfo(Image &image);
};

// TODO: 临时方案，该函数含有vkQueueSubmit，都用统一锁调用，避免多线程冲突
void endSingleTimeCommands_helper(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue);

#endif //PRF_IMAGEMANAGER_H
