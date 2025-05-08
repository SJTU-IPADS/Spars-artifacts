//
// Created by richardwu on 10/30/24.
//

#ifndef PRF_ENGINE2D_H
#define PRF_ENGINE2D_H

#include "BufferManager.h"
#include "PipelineManager.h"
#include "ImageManager.h"
#include "GlyphManager.h"
#include "SamplerDescriptorManager.h"
#include "DrawTask.h"
#include "DrawResource.h"
#include "paint/Paint.h"
#include "rects/Rect.h"
#include "circles/Circle.h"
#include "text/Text.h"
#include "rrects/RRect.h"
#include "../vulkan/utils.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vector>

/* 所有绘制接口类
 * 该类函数的调用位于工作线程 */
class Engine2D {
public:
    static void init(android_app *androidAppCtx, VulkanDeviceInfo* deviceInfo, VulkanSwapchainInfo* swapchainInfo, VulkanRenderInfo* renderInfo); // 创建
    static void del(); // 删除

    static void resetFrame(uint32_t frameIndex); // 每帧开始时，重置上一次轮转的资源

    /**
     * 绘制一系列的长方形
     * @param rects 长方形信息
     * @param paints 绘制样式，与rects一一对应
     * @return 已生成的资源 TODO: 改为unique_pointer
     */
    static DrawResource drawRects(std::vector<Rect> &rects, std::vector<Paint> &paints);

    /**
     * 绘制一系列的圆形
     * @param circles 圆形信息
     * @param paints 绘制样式，与circles一一对应
     * @return 已生成的资源 TODO: 改为unique_pointer
     */
    static DrawResource drawCircles(std::vector<Circle> &circles, std::vector<Paint> &paints);

    /**
    * 绘制一张矩形图片
    * @param image 图片信息（图片内包含了矩形）
    * @param paint 绘制样式
    * @return 已生成的资源 TODO: 改为unique_pointer
    */
    static DrawResource drawImage(Image &image, Paint &paint);

    /**
     * 绘制一系列的文本
     * @param texts 文本信息
     * @param paints 绘制样式，与texts一一对应
     * @return 已生成的资源 TODO: 改为unique_pointer
     */
    static DrawResource drawTexts(std::vector<Text> &texts, std::vector<Paint> &paints);

    /**
     * 绘制一系列的圆角长方形
     * @param rrects 圆角长方形信息
     * @param paints 绘制样式，与rrects一一对应
     * @return 已生成的资源 TODO: 改为unique_pointer
     */
    static DrawResource drawRRects(std::vector<RRect> &rrects, std::vector<Paint> &paints);

private:
    static uint32_t frameIndex_; // 当前正在绘制的轮转帧

    // 以下变量生命周期维护在Engine2D内部
    /* 管理系统全局的所有各类型的VkBuffer */
    static BufferManager *vertexBufferManager_;
    static BufferManager *indexBufferManager_;

    /* 管理系统全局所有的VkPipeline */
    static PipelineManager *pipelineManager_;

    /* 管理系统全局所有的VkImage */
    static ImageManager *imageManager_;
    static GlyphManager *glyphManager_;

    /* 管理系统中所有的ImageSamplerDescriptor */
    static SamplerDescriptorManager *samplerDescriptorManager_;

    // 以下变量生命周期在Engine2D外部，Engine2D仅拿取指针使用
    static android_app *androidAppCtx_;
    static VulkanDeviceInfo *deviceInfo_;
    static VulkanSwapchainInfo *swapchainInfo_;
    static VulkanRenderInfo *renderInfo_;

    // 将屏幕像素坐标转换为vulkan[-1,1]坐标
    static float coordinateXToVulkan(float x);
    static float coordinateYToVulkan(float y);
};


#endif //PRF_ENGINE2D_H
