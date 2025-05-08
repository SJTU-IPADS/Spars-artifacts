//
// Created by richardwu on 11/18/24.
//

#include "GlyphManager.h"
#include "../log.h"

#include "stb_image.h"

#include "PipelineManager.h"
#include "../vulkan/utils.h"

#include <stdexcept>
#include <algorithm>
#include <android/trace.h>

// 使用freetype
#include "ft2build.h"
#include FT_FREETYPE_H

std::size_t TextHash::operator()(const Text& obj) const {
    std::size_t h1 = std::hash<std::string>()(obj.fontPath_);
    std::size_t h2 = std::hash<float>()(obj.pixelHeight_);

    // 组合哈希值（常见方法之一）
    return h1 ^ (h2 << 1); // 使用位移和异或来混合哈希值
}

GlyphManager::GlyphManager(android_app *app, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue) {
    app_ = app;
    device_ = device;
    physicalDevice_ = physicalDevice;
    commandPool_ = commandPool;
    graphicsQueue_ = graphicsQueue;
}

GlyphManager::~GlyphManager() {
    std::unique_lock<std::shared_mutex> locker(mutex_);
    for(auto iter = fontMap_.begin(); iter != fontMap_.end(); iter++) {
        free(iter->second.glyphTop_);
        free(iter->second.glyphRight_);
        free(iter->second.glyphBottom_);
        free(iter->second.glyphLeftOffsets_);
        free(iter->second.glyphTopOffsets_);

        VulkanImageInfo imageInfo = iter->second.atlasInfo_;

        // 销毁采样器 TODO: 后续和image解耦开
        vkDestroySampler(device_, imageInfo.textureSampler_, nullptr);
        // 销毁纹理图像
        vkDestroyImageView(device_, imageInfo.textureImageView_, nullptr);
        vkDestroyImage(device_, imageInfo.textureImage_, nullptr);
        vkFreeMemory(device_, imageInfo.textureImageMemory_, nullptr);

    }
}

bool GlyphManager::findTextTmageInfo(Text &text, GlyphInfo *glyphInfo) {
    {
        std::shared_lock<std::shared_mutex> locker(mutex_);
        auto iter = fontMap_.find(text);
        if (iter != fontMap_.end()) {
            *glyphInfo = iter->second;
            return true;
        }
    }
    {
        std::unique_lock<std::shared_mutex> locker(mutex_);
        // 未找到，判断是否有其他线程正在创建
        auto iter2 = preparingMap_.find(text);
        if (iter2 != preparingMap_.end()) { // 其他线程正在创建
            cv_.wait(locker, [this, &text] {
                return preparingMap_.find(text) == preparingMap_.end(); // 该资源已经不在准备列表中了
            });

            // 其他线程创建完毕了
            auto iter3 = fontMap_.find(text);
            if (iter3 != fontMap_.end()) {
                *glyphInfo = iter3->second;
                return true;
            }
            // 此处不应该走到
            throw std::runtime_error("GlyphManager::findTextImageInfo error!");
            return false;
        } else {
            // 我们是第一个创建的
            preparingMap_[text] = true; // 将该资源加入准备列表，这个true不重要
            return false; // 需要我们后续创建
        }

    }
}

void GlyphManager::createAndInsertTextImageInfo(Text &text, GlyphInfo *glyphInfo) {
    *glyphInfo = createFontImageInfo(text); // 解压过程很长，无需锁保护

    std::unique_lock<std::shared_mutex> locker(mutex_);
    fontMap_[text] = *glyphInfo;

    // 资源创建完成，从准备列表移除
    preparingMap_.erase(text);
    cv_.notify_all(); // 此唤醒会导致所有任务唤醒，尽管有些任务在等待其他资源创建完成
}

// ================================== 以下为一些辅助函数 ==================================
/**
 * 查找合适的显存类型
 */
static uint32_t findMemoryType_helper(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{

    // 物理设备可用的内存类型
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


/**
 * 创建缓冲的辅助函数
 * 可以使用不同的大小、usage、properties
 */
static void createBuffer_helper(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                                VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType_helper(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

// 创建图像及图像内存
static void createImage_helper(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                               VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
{
    // 创建图像
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling; // VK_IMAGE_TILING_OPTIMAL以对访问优化的方式排列。VK_IMAGE_TILING_LINEAR，纹素以行主序的方式排列
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;                           // 作为接收方、会被着色器采样
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // 图像对象只被传输队列族使用
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;         // 多重采样（只对用作附着的图像有效，这里无关）
    imageInfo.flags = 0;                               // Optional

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    // 分配图像内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements); // 获取图像内存需求

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType_helper(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0); // 将图像对象和内存相关联
}

// 从指令池中分配指令缓冲，并开始记录
// TODO: commandPool需要单独管理并加锁
static VkCommandBuffer beginSingleTimeCommands_helper(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool; // TODO: 我们使用同样的指令池（用独立的会性能更好，因为内存传输指令的指令缓冲通常生命周期很短暂）
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // 开始记录内存传输指令
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // 只使用该指令一次

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

//// 结束记录、提交指令缓冲、并等待完成
//static void endSingleTimeCommands_helper(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
//{
//    // 结束记录
//    vkEndCommandBuffer(commandBuffer);
//
//    // 提交指令缓冲，完成传输
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers = &commandBuffer;
//
//    // TODO: 这里会出现多线程使用一个graphicsQueue提交指令，可能会有冲突，需单独拿出来管理
//    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
//    vkQueueWaitIdle(graphicsQueue); // 等待 // TODO: 等待看样子是唯一办法？
//
//    // 传输完成，清除指令缓冲
//    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
//}

// 变换图像布局
static void transitionImageLayout_helper(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format,
                                         VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands_helper(device, commandPool);

    VkImageMemoryBarrier barrier = {}; // 管线屏障，保障诸如图像在读取之前被写入（缓冲对象也有类似的）
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // 传递队列所有权时使用

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1; // 我们的图形不存在细分级别

    barrier.srcAccessMask = 0; // later
    barrier.dstAccessMask = 0; // later

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // 传输的变换
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0; // 写入操作不需等待任何对象
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;   // 最早出现的管线阶段
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // 伪阶段。传输阶段
    }
        // 采样前的变换
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // 片段着色器管线阶段的读取访问掩码
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands_helper(commandBuffer, device, commandPool, graphicsQueue);
}

// 复制缓冲到图像
static void copyBufferToImage_helper(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                                     VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands_helper(device, commandPool);

    // 指定将数据复制到图像那一部分
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // 在内存中紧凑存放
    region.bufferImageHeight = 0;
    // 下面是指定复制到图像中的哪一部分
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands_helper(commandBuffer, device, commandPool, graphicsQueue);
}


/**
 * 根据要求创建图像视图
 */
static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    // 指定图像数据的解释方式
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 二维纹理
    viewInfo.format = format;

    // 使用默认的颜色通道映射（比如单色纹理可以将所有颜色通道映射到红色）
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // subresourceRange指定图像用途和哪一部分可以被使用
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}


// 创建采样器对象
static VkSampler createTextureSampler(VkDevice device)
{
    // 指定采样器使用的过滤和变换操作
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // 超出图像范围时重复纹理（像地转平铺纹理）
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // 各向异性过滤
    samplerInfo.anisotropyEnable = VK_FALSE;
    // samplerInfo.maxAnisotropy = 16;

    // 边界颜色，只有当addressModeX是VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER才有用
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE; // 为VK_FALSE时用[0,1)采样，不然则为[0,texWidth或Height)

    samplerInfo.compareEnable = VK_FALSE; // 和预设值进行比较，在阴影贴图时使用
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler textureSampler;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return textureSampler;
}

// ================================== 以上为一些辅助函数 ==================================



// 加载字体atlas到一个VkImage
GlyphInfo GlyphManager::createFontImageInfo(Text &text)
{
    ATrace_beginSection(("createFontImageInfo: " + text.fontPath_).c_str());

    // 返回值
    GlyphInfo glyphInfo;
    glyphInfo.glyphTop_ = (float*)malloc(sizeof(float) * ASCII_CHAR_COUNT);
    glyphInfo.glyphRight_ = (float*)malloc(sizeof(float) * ASCII_CHAR_COUNT);
    glyphInfo.glyphBottom_ = (float*)malloc(sizeof(float) * ASCII_CHAR_COUNT);
    glyphInfo.glyphLeftOffsets_ = (int*) malloc(sizeof(int) * ASCII_CHAR_COUNT);
    glyphInfo.glyphTopOffsets_ = (int*) malloc(sizeof(int) * ASCII_CHAR_COUNT);

    // freetype库
    FT_Library library;
    if (FT_Init_FreeType(&library)) {
        throw std::runtime_error("freetype init library error!");
    }

    // freetype字体
    FT_Face face;
    if (FT_New_Face(library, text.fontPath_.c_str(), 0, &face)) {
        throw std::runtime_error("freetype load font error!");
    }

    // 设置加载字体大小
    FT_Set_Pixel_Sizes(face, 0, text.pixelHeight_);

    // TODO: 当前为了简单管理，每个glyph在atlas中都占据相同的大小。首先迭代一遍所有字符获得最大长宽
    int maxGlyphHeight = 0;
    int maxGlyphWidth = 0;

    // 获取所有字符的最大长宽
    for (int i = 0; i < ASCII_CHAR_COUNT; i++) {
        wchar_t c = i; // wchar_t可以后续支持unicode

        FT_Load_Char(face, c, FT_LOAD_DEFAULT); // 此处没有渲染
        FT_GlyphSlot slot = face->glyph;
        auto bitmap = &slot->bitmap;
        int rows = bitmap->rows; // 获得字高
        int cols = bitmap->width; // 获得字宽
        glyphInfo.glyphLeftOffsets_[i] = slot->bitmap_left; // 对齐使用
        glyphInfo.glyphTopOffsets_[i] = slot->bitmap_top;

        maxGlyphHeight = std::max(maxGlyphHeight, rows);
        maxGlyphWidth = std::max(maxGlyphWidth, cols);
    }
    glyphInfo.maxGlyphHeight_ = maxGlyphHeight;
    glyphInfo.maxGlyphWidth_ = maxGlyphWidth;

    // 纹理为纵向纹理，宽度为maxGlyphWidth，高度为maxGlyphHeight * 128
    auto texWidth = maxGlyphWidth;
    auto texHeight = maxGlyphHeight * ASCII_CHAR_COUNT;
    VkDeviceSize imageSize = texHeight * texWidth;


    // 创建图像数据的stagingBuffer（暂存缓冲）
    ATrace_beginSection("renderAndCopyGlyphsToStageBuffer");
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer_helper(device_, physicalDevice_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &data);

    for (int i = 0; i < ASCII_CHAR_COUNT; i++) {
        wchar_t c = i; // wchar_t可以后续支持unicode

        /* FT先根据输入，获得Unicode后获得对应的glyph id。
        * 接着用glyph id渲染出了一张bitmap
        */
        FT_Load_Char(face, c, FT_LOAD_RENDER);
        FT_GlyphSlot slot = face->glyph; // 输出结果在 slot->bitmap

        auto bitmap = &slot->bitmap;
        int rows = bitmap->rows; // 获得字高
        int cols = bitmap->width; // 获得字宽

        unsigned char *base = (unsigned char *)data + i * maxGlyphWidth * maxGlyphHeight;

        // 为Engine2D采用atlas，normalize到1
        glyphInfo.glyphTop_[i] = (float)(i * maxGlyphHeight) / (float)texHeight;
        glyphInfo.glyphBottom_[i] = (float)(i * maxGlyphHeight + rows) / (float)texHeight;
        glyphInfo.glyphRight_[i] = (float)(cols) / (float)texWidth;

        // copy every line
        for (int j = 0; j < rows; j++) {
            memcpy(base + j * maxGlyphWidth, bitmap->buffer + j * cols, cols);
            memset(base + j * maxGlyphWidth + cols, 0, maxGlyphWidth - cols);
        }
        // set the rest of lines to be all 0s
        memset(base + maxGlyphWidth * rows, 0, maxGlyphWidth * maxGlyphHeight - maxGlyphWidth * rows); // 将剩下的字节设置为0

    }

    vkUnmapMemory(device_, stagingBufferMemory);
    ATrace_endSection();

    createImage_helper(device_, physicalDevice_, texWidth, texHeight, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, glyphInfo.atlasInfo_.textureImage_, glyphInfo.atlasInfo_.textureImageMemory_);

    ATrace_beginSection("firstTransition");
    // 变换纹理图像到VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout_helper(device_, commandPool_, graphicsQueue_, glyphInfo.atlasInfo_.textureImage_, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED /* 创建时initialLayout */, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    ATrace_endSection();

    ATrace_beginSection("copyBufferToImage");
    // 执行图像数据复制操作
    copyBufferToImage_helper(device_, commandPool_, graphicsQueue_, stagingBuffer, glyphInfo.atlasInfo_.textureImage_, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    ATrace_endSection();

    ATrace_beginSection("secondTransition");
    // 为了能够在着色器中采样纹理图像数据，还需进行一次变换
    transitionImageLayout_helper(device_, commandPool_, graphicsQueue_, glyphInfo.atlasInfo_.textureImage_, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ATrace_endSection();

    // 清理
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);

    // 创建图像视图
    glyphInfo.atlasInfo_.textureImageView_ = createImageView(device_, glyphInfo.atlasInfo_.textureImage_, VK_FORMAT_R8_UNORM);

    // 创建采样器 // TODO: 采样器和图片本身不耦合，后续应该和paint耦合，使用哈希表查找
    glyphInfo.atlasInfo_.textureSampler_ = createTextureSampler(device_);

    // 释放资源
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    ATrace_endSection();

    return glyphInfo;
}
