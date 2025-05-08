//
// Created by richardwu on 10/31/24.
//

#ifndef PRF_DRAWTASK_H
#define PRF_DRAWTASK_H

#include <cstdint>
#include <string>
#include <vector>

#include "DrawResource.h"
#include "pipeline_helper.h"
#include "paint/Paint.h"
#include "rects/Rect.h"
#include "circles/Circle.h"
#include "image/Image.h"
#include "text/Text.h"
#include "rrects/RRect.h"
#include "../renderTree/DrawCmd.h"
#include "../renderTree/RenderNode.h"

#define BATCH_SUCCESSFUL 0
#define BATCH_FAIL_OVERLAP 1
#define BATCH_FAIL_NO_OVERLAP 2

class DrawCmd;
class RenderNode;
class DrawResource;

enum DrawTaskType : uint16_t
{
    RECTS_DRAWTASK,
    CIRCLES_DRAWTASK,
    IMAGE_DRAWTASK,
    TEXTS_DRAWTASK,
    RRECTS_DRAWTASK,
};

std::string getDrawTaskTypeString(DrawTaskType type);

/* 一个绘制任务需要的资源（输入）*/
class DrawTask {
private:
    uint32_t taskId_; // TODO: 当前收集者按照taskId_的顺序线性收集
public:
    DrawTask(uint32_t taskId, Rect &boundingBox);
    uint32_t getTaskId();
    std::string getTypeName();

    /* 不同种类的DrawTask需要重写的函数 */
    virtual DrawTaskType getType() = 0;
    virtual DrawResource draw() = 0;

    // 该任务的绘制范围，为了合批和乱序插入
    Rect boundingBox_; // TODO: public for convenience

    /**
     * 合批一个DrawCmd，若不重写，默认不可以合批
     * @param drawCmd 要合批的指令
     * @param cmdBoundingBox 该指令的boundingBox
     * @return BATCH_SUCCESSFUL 或 BATCH_FAIL_OVERLAP 或 BATCH_FAIL_NO_OVERLAP
     */
    virtual uint32_t batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode);
};

class RectsDrawTask : public DrawTask
{
private:
    std::vector<Rect> rects_;
    std::vector<Paint> paints_;
public:
    RectsDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<Rect> &rects, std::vector<Paint> &paints);
    DrawTaskType getType() override;
    DrawResource draw() override;
    uint32_t batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode) override;
    static void createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo);
};

class CirclesDrawTask : public DrawTask
{
private:
    std::vector<Circle> circles_;
    std::vector<Paint> paints_;
public:
    CirclesDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<Circle> &circles, std::vector<Paint> &paints);
    DrawTaskType getType() override;
    DrawResource draw() override;
    uint32_t batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode) override;
    static void createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                       VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo);
};

class ImageDrawTask : public DrawTask
{
private:
    Paint paint_;
public:
    Image image_; // TODO: for convenience
    ImageDrawTask(uint32_t taskId, Rect &boundingBox, Image &image, Paint &paint);
    DrawTaskType getType() override;
    DrawResource draw() override;
    // uint32_t batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode) override; // 图片绘制无法合批
//    static void createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
//                                       VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo);
};

class TextsDrawTask : public DrawTask
{
private:
    std::vector<Text> texts_;
    std::vector<Paint> paints_;
public:
    TextsDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<Text> &texts, std::vector<Paint> &paints);
    DrawTaskType getType() override;
    DrawResource draw() override;
    uint32_t batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode) override;
//    static void createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
//                                       VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo);
};

class RRectsDrawTask : public DrawTask
{
private:
    std::vector<RRect> rrects_;
    std::vector<Paint> paints_;
public:
    RRectsDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<RRect> &rrects, std::vector<Paint> &paints);
    DrawTaskType getType() override;
    DrawResource draw() override;
    uint32_t batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode) override;
    static void createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                       VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo);
};

#endif //PRF_DRAWTASK_H
