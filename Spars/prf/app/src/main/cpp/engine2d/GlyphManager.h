//
// Created by richardwu on 11/24/24.
//

#ifndef PRF_GLYPHMANAGER_H
#define PRF_GLYPHMANAGER_H

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan_wrapper.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <condition_variable>

#include "ImageManager.h"
#include "text/Text.h"

// 当前将前128个字符放入atlas
#define ASCII_CHAR_COUNT 128

struct TextHash {
    std::size_t operator()(const Text& obj) const;
};

// 图片（纹理）管理信息
struct GlyphInfo {
    VulkanImageInfo atlasInfo_;
    float *glyphTop_; // 记录每个glyph在atlas中top的位置
    float *glyphRight_; // 记录每个glyph在atlas中right的位置（left 始终为0）
    float *glyphBottom_; // 记录每个glyph在atlas中bottom的位置
    int *glyphLeftOffsets_; // 用于绘制对齐
    int *glyphTopOffsets_;
    int maxGlyphHeight_; // 记录最大字符像素高度
    int maxGlyphWidth_; // 记录最大字符像素宽度
};

/*
 * 管理所有的字体纹理VulkanImageInfo
 * TODO: 当前只有一张字体，后续支持多字体
 */
class GlyphManager {
public:
    GlyphManager(android_app *app, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~GlyphManager(); // 删除所有VkImage

    void createAndInsertTextImageInfo(Text &text, GlyphInfo *glyphInfo); // 创建并插入字体的VulkanImageInfo，创建的值通过imageInfo参数返回

    /**
     * 查找是否有缓存的资源：
     *  若有：      返回该资源（通过参数）
     *  若没有：    若当前正有其他并发任务在创建该资源，则阻塞，在其创建完成后返回该资源
     *             若当前为第一个使用该未创建资源的任务，则直接返回false，并将该资源列为“准备中”（其他并发任务会阻塞）
     * @return 是否有缓存的资源
     */
    bool findTextTmageInfo(Text &text, GlyphInfo *glyphInfo); // 返回是否找到

private:

    std::shared_mutex mutex_; // 保护下面的map
    std::condition_variable_any cv_; // 确保同时只有一个任务在创建资源
    std::unordered_map<Text, GlyphInfo, TextHash> fontMap_;
    std::unordered_map<Text, bool, TextHash> preparingMap_; // 维护所有正在创建的Font Image，避免多任务并发导致的重复创建

    android_app *app_;
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkCommandPool commandPool_;
    VkQueue graphicsQueue_;


    // 创建一张图片所有的内容
    GlyphInfo createFontImageInfo(Text &text);
};


#endif //PRF_GLYPHMANAGER_H
