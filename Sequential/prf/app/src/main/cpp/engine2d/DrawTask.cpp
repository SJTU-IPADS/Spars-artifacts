//
// Created by richardwu on 11/6/24.
//

#include "DrawTask.h"
#include "Engine2D.h"

std::string getDrawTaskTypeString(DrawTaskType type)
{
#define DRAWTASKTYPESTRING(t) \
    case (t):                \
        return #t
    switch (type)
    {
        DRAWTASKTYPESTRING(RECTS_DRAWTASK);
        DRAWTASKTYPESTRING(CIRCLES_DRAWTASK);
        DRAWTASKTYPESTRING(IMAGE_DRAWTASK);
        DRAWTASKTYPESTRING(TEXTS_DRAWTASK);
        DRAWTASKTYPESTRING(RRECTS_DRAWTASK);
        default:
            return "DRAWTASK_UNKNOWN";
    }
}

DrawTask::DrawTask(uint32_t taskId, Rect &boundingBox): taskId_(taskId), boundingBox_(boundingBox) {
}

uint32_t DrawTask::getTaskId() {
    return taskId_;
}

std::string DrawTask::getTypeName()
{
    return getDrawTaskTypeString(getType());
}

// 默认不可以合批，ImageDrawTask可采用这个默认函数
uint32_t DrawTask::batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode) {
    if (isOverlap(cmdBoundingBox, boundingBox_)) {
        return BATCH_FAIL_OVERLAP;
    } else {
        return BATCH_FAIL_NO_OVERLAP;
    }
}

// 长方形绘制任务
RectsDrawTask::RectsDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<Rect> &rects, std::vector<Paint> &paints) : DrawTask(taskId, boundingBox) {
    rects_ = rects;
    paints_ = paints;
}

DrawTaskType RectsDrawTask::getType() {
    return RECTS_DRAWTASK;
}

DrawResource RectsDrawTask::draw() {
    return Engine2D::drawRects(rects_, paints_);
}

uint32_t RectsDrawTask::batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode)
{
    if(drawCmd->getType() == RECT_DRAWCMD) {
        // 可以合批
        RectDrawCmd *rectDrawCmd = dynamic_cast<RectDrawCmd*>(drawCmd);
        rects_.push_back(Rect::MakeXYWH(renderNode->getAbsX() + rectDrawCmd->rect_.x_,
                                       renderNode->getAbsY() + rectDrawCmd->rect_.y_,
                                       rectDrawCmd->rect_.w_,
                                       rectDrawCmd->rect_.h_));
        paints_.push_back(drawCmd->getPaint());

        // 新的包围矩形
        boundingBox_ = getLargerRect(cmdBoundingBox, boundingBox_);

        return BATCH_SUCCESSFUL;
    }
    // 不同类型不可合批
    else {
        return DrawTask::batchWith(drawCmd, cmdBoundingBox, renderNode);
    }
}

void RectsDrawTask::createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                  VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo) {
    char vsFilePath[] = "shaders/rects.vert.spv";
    char fsFilePath[] = "shaders/rects.frag.spv";
    createGraphicsPipelineHelper(androidAppCtx, device, extent2D, renderPass, pipelineInfo, vsFilePath, fsFilePath);
}


// 圆形绘制任务
CirclesDrawTask::CirclesDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<Circle> &circles, std::vector<Paint> &paints) : DrawTask(taskId, boundingBox) {
    circles_ = circles;
    paints_ = paints;
}

DrawTaskType CirclesDrawTask::getType() {
    return CIRCLES_DRAWTASK;
}

DrawResource CirclesDrawTask::draw() {
    return Engine2D::drawCircles(circles_, paints_);
}

uint32_t CirclesDrawTask::batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode)
{
    if(drawCmd->getType() == CIRCLE_DRAWCMD) {
        // 可以合批
        CircleDrawCmd *circleDrawCmd = dynamic_cast<CircleDrawCmd*>(drawCmd);
        circles_.push_back(Circle::MakeXYR(renderNode->getAbsX() + circleDrawCmd->circle_.x_,
                                          renderNode->getAbsY() + circleDrawCmd->circle_.y_,
                                           circleDrawCmd->circle_.r_));
        paints_.push_back(drawCmd->getPaint());

        // 新的包围矩形
        boundingBox_ = getLargerRect(cmdBoundingBox, boundingBox_);

        return BATCH_SUCCESSFUL;
    }
        // 不同类型不可合批
    else {
        return DrawTask::batchWith(drawCmd, cmdBoundingBox, renderNode);
    }
}

void CirclesDrawTask::createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                             VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo) {
    char vsFilePath[] = "shaders/circles.vert.spv";
    char fsFilePath[] = "shaders/circles.frag.spv";
    createGraphicsPipelineHelper(androidAppCtx, device, extent2D, renderPass, pipelineInfo, vsFilePath, fsFilePath);
}


// 图片绘制任务
ImageDrawTask::ImageDrawTask(uint32_t taskId, Rect &boundingBox, Image &image, Paint &paint) : DrawTask(taskId, boundingBox), image_(image) {
    paint_ = paint;
}

DrawTaskType ImageDrawTask::getType() {
    return IMAGE_DRAWTASK;
}

DrawResource ImageDrawTask::draw() {
    return Engine2D::drawImage(image_, paint_);
}


// 文本绘制任务
TextsDrawTask::TextsDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<Text> &texts, std::vector<Paint> &paints): DrawTask(taskId, boundingBox) {
    texts_ = texts;
    paints_ = paints;
}

DrawTaskType TextsDrawTask::getType() {
    return TEXTS_DRAWTASK;
}

DrawResource TextsDrawTask::draw() {
    return Engine2D::drawTexts(texts_, paints_);
}

uint32_t TextsDrawTask::batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode)
{
    if(drawCmd->getType() == TEXT_DRAWCMD) {
        TextDrawCmd *textDrawCmd = dynamic_cast<TextDrawCmd*>(drawCmd);

        // 只有相同字体和相同pixelHeight才可以合批（因为VkPipeline可以采样同一张atlas）
        if (texts_.empty() ||
            (textDrawCmd->text_.fontPath_ == texts_[0].fontPath_ &&
            textDrawCmd->text_.pixelHeight_ == texts_[0].pixelHeight_)) {
            texts_.push_back(Text::MakeText(renderNode->getAbsX() + textDrawCmd->text_.x_,
                                            renderNode->getAbsY() + textDrawCmd->text_.y_,
                                            textDrawCmd->text_.pixelHeight_,
                                            textDrawCmd->text_.str_,
                                            textDrawCmd->text_.fontPath_));
            paints_.push_back(drawCmd->getPaint());

            // 新的包围矩形
            boundingBox_ = getLargerRect(cmdBoundingBox, boundingBox_);
            return BATCH_SUCCESSFUL;
        }
    }

    // 不同类型不可合批
    return DrawTask::batchWith(drawCmd, cmdBoundingBox, renderNode);

}


// 圆角长方形绘制任务
RRectsDrawTask::RRectsDrawTask(uint32_t taskId, Rect &boundingBox, std::vector<RRect> &rrects, std::vector<Paint> &paints) : DrawTask(taskId, boundingBox) {
    rrects_ = rrects;
    paints_ = paints;
}

DrawTaskType RRectsDrawTask::getType() {
    return RRECTS_DRAWTASK;
}

DrawResource RRectsDrawTask::draw() {
    return Engine2D::drawRRects(rrects_, paints_);
}

uint32_t RRectsDrawTask::batchWith(DrawCmd *drawCmd, Rect &cmdBoundingBox, RenderNode *renderNode)
{
    if(drawCmd->getType() == RRECT_DRAWCMD) {
        // 可以合批
        RRectDrawCmd *rrectDrawCmd = dynamic_cast<RRectDrawCmd*>(drawCmd);
        rrects_.push_back(RRect::MakeXYWHR(renderNode->getAbsX() + rrectDrawCmd->rrect_.x_,
                                        renderNode->getAbsY() + rrectDrawCmd->rrect_.y_,
                                        rrectDrawCmd->rrect_.w_,
                                        rrectDrawCmd->rrect_.h_,
                                        rrectDrawCmd->rrect_.r_));
        paints_.push_back(drawCmd->getPaint());

        // 新的包围矩形
        boundingBox_ = getLargerRect(cmdBoundingBox, boundingBox_);

        return BATCH_SUCCESSFUL;
    }
        // 不同类型不可合批
    else {
        return DrawTask::batchWith(drawCmd, cmdBoundingBox, renderNode);
    }
}

void RRectsDrawTask::createGraphicsPipeline(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                           VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo) {
    char vsFilePath[] = "shaders/rrects.vert.spv";
    char fsFilePath[] = "shaders/rrects.frag.spv";
    createGraphicsPipelineHelperRRect(androidAppCtx, device, extent2D, renderPass, pipelineInfo, vsFilePath, fsFilePath);
}