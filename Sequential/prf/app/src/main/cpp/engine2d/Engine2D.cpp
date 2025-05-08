//
// Created by richardwu on 10/30/24.
//

#include "Engine2D.h"

#include <cmath>

// 静态变量初始化
uint32_t Engine2D::frameIndex_;
BufferManager *Engine2D::vertexBufferManager_;
BufferManager *Engine2D::indexBufferManager_;
PipelineManager *Engine2D::pipelineManager_;
ImageManager *Engine2D::imageManager_;
GlyphManager *Engine2D::glyphManager_;
SamplerDescriptorManager *Engine2D::samplerDescriptorManager_;

android_app *Engine2D::androidAppCtx_;
VulkanDeviceInfo *Engine2D::deviceInfo_;
VulkanSwapchainInfo *Engine2D::swapchainInfo_;
VulkanRenderInfo *Engine2D::renderInfo_;

void Engine2D::init(android_app *androidAppCtx, VulkanDeviceInfo* deviceInfo, VulkanSwapchainInfo* swapchainInfo, VulkanRenderInfo* renderInfo) {

    androidAppCtx_ = androidAppCtx;
    deviceInfo_ = deviceInfo;
    swapchainInfo_ = swapchainInfo;
    renderInfo_ = renderInfo;

    // 为每个2的整次幂维护一个可用VkBuffer的列表进行复用（全局数据结构）
    vertexBufferManager_ = new BufferManager(deviceInfo->device_, deviceInfo->physicalDevice_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBufferManager_ = new BufferManager(deviceInfo->device_, deviceInfo->physicalDevice_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    // 维护所有的VkPipeline
    pipelineManager_ = new PipelineManager(deviceInfo->device_);

    // 维护所有的VkImage
    imageManager_ = new ImageManager(androidAppCtx, deviceInfo->device_, deviceInfo->physicalDevice_, renderInfo->cmdPool_, deviceInfo_->queue_);

    // 维护所有的字体atlas
    glyphManager_ = new GlyphManager(androidAppCtx, deviceInfo->device_, deviceInfo->physicalDevice_, renderInfo->cmdPool_, deviceInfo_->queue_);

    // 维护所有sampler相关的descriptor
    samplerDescriptorManager_ = new SamplerDescriptorManager(deviceInfo->device_);
}

void Engine2D::del() {
    // 删除所有VkImage相关，包括sampler, imageView, image, imageMemory
    delete imageManager_;
    delete glyphManager_;

    // 删除descriptorPool
    delete samplerDescriptorManager_;

    // 删除所有VkPipeline和VkPipelineLayout
    delete pipelineManager_;

    // 调用析构函数，释放VkBuffer与VkDeviceMemory
    delete vertexBufferManager_;
    delete indexBufferManager_;
}

void Engine2D::resetFrame(uint32_t frameIndex) {
    frameIndex_ = frameIndex;
    vertexBufferManager_->freeAllBuffers(frameIndex);
    indexBufferManager_->freeAllBuffers(frameIndex);
//  vertexBufferManager_->dump();
//  indexBufferManager_->dump();
}

DrawResource Engine2D::drawRects(std::vector<Rect> &rects, std::vector<Paint> &paints) {

    // 检查
    if (rects.size() != paints.size()) {
        LOGE("drawRects inputs do not match!");
    }

    // 该任务生成的绘制资源
    DrawResource drawResource;

    // 创建VkPipeline（如果未曾被创建过）
    if(!pipelineManager_->findPipeline(RECT_PIPELINE, &drawResource.pipelineInfo_)) {
        // 未曾创建过，则创建
        RectsDrawTask::createGraphicsPipeline(androidAppCtx_, deviceInfo_->device_,
                               swapchainInfo_->displaySize_, renderInfo_->renderPass_,
                               &drawResource.pipelineInfo_);
        pipelineManager_->insertPipeline(RECT_PIPELINE, drawResource.pipelineInfo_);
    }

    // 生成vertex和index数据
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    for(int i = 0; i < rects.size(); i++) {
        // 插入顶点坐标
        float left = coordinateXToVulkan(rects[i].x_);
        float top = coordinateYToVulkan(rects[i].y_);
        float right = coordinateXToVulkan(rects[i].x_ + rects[i].w_);
        float bottom = coordinateYToVulkan(rects[i].y_ + rects[i].h_);
        vertexData.push_back(left);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(right);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(right);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(left);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);

        uint32_t base = i * 4;
        indexData.push_back(base);
        indexData.push_back(base + 1);
        indexData.push_back(base + 2);
        indexData.push_back(base + 2);
        indexData.push_back(base + 3);
        indexData.push_back(base);
    }

    // 创建VkBuffer
    drawResource.vertexBufferInfo_ = vertexBufferManager_->allocBuffer(frameIndex_, sizeof(float) * vertexData.size());
    void *data;
    vkMapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_, 0, sizeof(float) * vertexData.size(),0, &data);
    memcpy(data, vertexData.data(), sizeof(float) * vertexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_);

    // 创建VkBuffer
    drawResource.indexBufferInfo_ = indexBufferManager_->allocBuffer(frameIndex_, sizeof(uint32_t) * indexData.size());
    vkMapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_, 0, sizeof(uint32_t) * indexData.size(),0, &data);
    memcpy(data, indexData.data(), sizeof(uint32_t) * indexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_);

    drawResource.indexCount_ = indexData.size();

    return drawResource;
}

DrawResource Engine2D::drawImage(Image &image, Paint &paint) {

    // 该任务生成的绘制资源
    DrawResource drawResource;

    VulkanImageInfo imageInfo; // 该值不需要返回给收集线程，收集线程只需要descriptor即可

    // findImageInfo可能会阻塞（若其他任务正在创建）
    if (!imageManager_->findImageInfo(image, &imageInfo)) {
        // 未创建过该图像，则创建
        imageManager_->createAndInsertImageInfo(image, &imageInfo);
    }

    // 创建VkPipeline（如果未曾被创建过），findPipeline可能会阻塞（若其他任务正在创建）
    if(!pipelineManager_->findPipeline(IMAGE_PIPELINE, &drawResource.pipelineInfo_)) {
        // 未曾创建过，则创建
        VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayoutImage(
                deviceInfo_->device_); // descriptorSetLayout 会随着vkPipeline销毁

        // 该函数内包含 pipelineInfo->descriptorSetLayout_ = descriptorSetLayout; 因此后续可以使用
        createGraphicsPipelineHelperImage(androidAppCtx_, *deviceInfo_,
                                                                   *swapchainInfo_, *renderInfo_,
                                                                   &drawResource.pipelineInfo_,
                                                                   "shaders/image.vert.spv", "shaders/image.frag.spv", descriptorSetLayout);

        pipelineManager_->insertPipeline(IMAGE_PIPELINE, drawResource.pipelineInfo_);
    }

    // findSamplerDescriptor可能会阻塞（若其他任务正在创建）
    drawResource.descriptorSetInfo_.valid_ = true; // 此图元需使用descriptor
    if(!samplerDescriptorManager_->findSamplerDescriptor(imageInfo.textureImageView_, &drawResource.descriptorSetInfo_.descriptorSet_)) {
        // 未创建过该descriptor，则创建
        samplerDescriptorManager_->createAndInsertSamplerDescriptor(imageInfo.textureImageView_, drawResource.pipelineInfo_.descriptorSetLayout_, imageInfo, &drawResource.descriptorSetInfo_.descriptorSet_);
    }

    // 生成vertex和index数据
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    // 插入顶点坐标
    Rect &rect = image.rect_;

    float left = coordinateXToVulkan(rect.x_);
    float top = coordinateYToVulkan(rect.y_);
    float right = coordinateXToVulkan(rect.x_ + rect.w_);
    float bottom = coordinateYToVulkan(rect.y_ + rect.h_);
    vertexData.push_back(left);
    vertexData.push_back(top);
    vertexData.push_back(0.0);
    vertexData.push_back(0.0);
    vertexData.push_back(right);
    vertexData.push_back(top);
    vertexData.push_back(1.0);
    vertexData.push_back(0.0);
    vertexData.push_back(right);
    vertexData.push_back(bottom);
    vertexData.push_back(1.0);
    vertexData.push_back(1.0);
    vertexData.push_back(left);
    vertexData.push_back(bottom);
    vertexData.push_back(0.0);
    vertexData.push_back(1.0);

    indexData.push_back(0);
    indexData.push_back(1);
    indexData.push_back(2);
    indexData.push_back(2);
    indexData.push_back(3);
    indexData.push_back(0);

    // 创建VkBuffer
    drawResource.vertexBufferInfo_ = vertexBufferManager_->allocBuffer(frameIndex_, sizeof(float) * vertexData.size());
    void *data;
    vkMapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_, 0, sizeof(float) * vertexData.size(),0, &data);
    memcpy(data, vertexData.data(), sizeof(float) * vertexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_);

    // 创建VkBuffer
    drawResource.indexBufferInfo_ = indexBufferManager_->allocBuffer(frameIndex_, sizeof(uint32_t) * indexData.size());
    vkMapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_, 0, sizeof(uint32_t) * indexData.size(),0, &data);
    memcpy(data, indexData.data(), sizeof(uint32_t) * indexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_);

    drawResource.indexCount_ = indexData.size();

    return drawResource;
}

DrawResource Engine2D::drawCircles(std::vector<Circle> &circles, std::vector<Paint> &paints) {

    // 检查
    if (circles.size() != paints.size()) {
        LOGE("drawCircles inputs do not match!");
    }

    // 该任务生成的绘制资源
    DrawResource drawResource;

    // 创建VkPipeline（如果未曾被创建过）
    if(!pipelineManager_->findPipeline(CIRCLE_PIPELINE, &drawResource.pipelineInfo_)) {
        // 未曾创建过，则创建
        CirclesDrawTask::createGraphicsPipeline(androidAppCtx_, deviceInfo_->device_,
                                     swapchainInfo_->displaySize_, renderInfo_->renderPass_,
                                     &drawResource.pipelineInfo_);
        pipelineManager_->insertPipeline(CIRCLE_PIPELINE, drawResource.pipelineInfo_);
    }

    // 生成vertex和index数据
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    int base = 0;

    for(int i = 0; i < circles.size(); i++) {

        int triCount = static_cast<int>(circles[i].r_ / 4); // 根据半径决定细分数量
        triCount = triCount > 20 ? triCount : 20; // 细分数量至少为20

        // 圆心
        vertexData.push_back(coordinateXToVulkan(circles[i].x_));
        vertexData.push_back(coordinateYToVulkan(circles[i].y_));
        vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);

        // 细分并插入顶点坐标
        for (int j = 0; j < triCount; j++) {
            double radians = 2 * M_PI / triCount * j; // 划分角度
            float x = circles[i].x_ + circles[i].r_ * static_cast<float>(cos(radians));
            float y = circles[i].y_ + circles[i].r_ * static_cast<float>(sin(radians));
            vertexData.push_back(coordinateXToVulkan(x));
            vertexData.push_back(coordinateYToVulkan(y));
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
        }

        // 组织三角形
        for(int j = 0; j < triCount - 1; j++) {
            indexData.push_back(base);
            indexData.push_back(base + j + 1);
            indexData.push_back(base + j + 2);
        }
        indexData.push_back(base);
        indexData.push_back(base + triCount);
        indexData.push_back(base + 1); // 最后一个三角形使用第一个顶点

        base += triCount + 1;
    }

    // 创建VkBuffer
    drawResource.vertexBufferInfo_ = vertexBufferManager_->allocBuffer(frameIndex_, sizeof(float) * vertexData.size());
    void *data;
    vkMapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_, 0, sizeof(float) * vertexData.size(),0, &data);
    memcpy(data, vertexData.data(), sizeof(float) * vertexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_);

    // 创建VkBuffer
    drawResource.indexBufferInfo_ = indexBufferManager_->allocBuffer(frameIndex_, sizeof(uint32_t) * indexData.size());
    vkMapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_, 0, sizeof(uint32_t) * indexData.size(),0, &data);
    memcpy(data, indexData.data(), sizeof(uint32_t) * indexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_);

    drawResource.indexCount_ = indexData.size();

    return drawResource;
}


DrawResource Engine2D::drawTexts(std::vector<Text> &texts, std::vector<Paint> &paints) {

    // 检查
    if (texts.size() != paints.size() || texts.empty()) {
        LOGE("drawTexts inputs do not match!");
    }

    // 该任务生成的绘制资源
    DrawResource drawResource;

    GlyphInfo glyphInfo; // 该值不需要返回给收集线程，收集线程只需要descriptor即可

    // findTextTmageInfo可能会阻塞（若其他任务正在创建），合批时已确认过只有相同字体才可以合批
    if (!glyphManager_->findTextTmageInfo(texts[0], &glyphInfo)) {
        // 未创建过该字体atlas，则创建
        glyphManager_->createAndInsertTextImageInfo(texts[0], &glyphInfo);
    }

    // 创建VkPipeline（如果未曾被创建过），findPipeline可能会阻塞（若其他任务正在创建）
    if(!pipelineManager_->findPipeline(TEXT_PIPELINE, &drawResource.pipelineInfo_)) {
        // 未曾创建过，则创建
        VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayoutImage(
                deviceInfo_->device_); // descriptorSetLayout 会随着vkPipeline销毁

        // 该函数内包含 pipelineInfo->descriptorSetLayout_ = descriptorSetLayout; 因此后续可以使用
        createGraphicsPipelineHelperText(androidAppCtx_, *deviceInfo_,
                                          *swapchainInfo_, *renderInfo_,
                                          &drawResource.pipelineInfo_,
                                          "shaders/text.vert.spv", "shaders/text.frag.spv", descriptorSetLayout);

        pipelineManager_->insertPipeline(TEXT_PIPELINE, drawResource.pipelineInfo_);
    }

    // findSamplerDescriptor可能会阻塞（若其他任务正在创建）
    drawResource.descriptorSetInfo_.valid_ = true; // 此图元需使用descriptor
    if(!samplerDescriptorManager_->findSamplerDescriptor(glyphInfo.atlasInfo_.textureImageView_, &drawResource.descriptorSetInfo_.descriptorSet_)) {
        // 未创建过该descriptor，则创建
        samplerDescriptorManager_->createAndInsertSamplerDescriptor(glyphInfo.atlasInfo_.textureImageView_, drawResource.pipelineInfo_.descriptorSetLayout_, glyphInfo.atlasInfo_, &drawResource.descriptorSetInfo_.descriptorSet_);
    }

    // 生成vertex和index数据
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    int count = 0;
    for(int i = 0; i < texts.size(); i++) {

        float horiAccPx = texts[i].x_;

        for(int j = 0; j < texts[i].str_.size(); j++) {

            char c = texts[i].str_.at(j);
            float leftSample = 0.0;
            float rightSample = glyphInfo.glyphRight_[c];
            float topSample = glyphInfo.glyphTop_[c];
            float bottomSample = glyphInfo.glyphBottom_[c];

            // 插入顶点坐标和纹理采样坐标
            float left = coordinateXToVulkan(horiAccPx + glyphInfo.glyphLeftOffsets_[c]);
            float top = coordinateYToVulkan(texts[i].y_ + texts[i].pixelHeight_ - glyphInfo.glyphTopOffsets_[c]); // TODO: 这里暂时加上texts[i].pixelHeight_
            float right = coordinateXToVulkan(horiAccPx + glyphInfo.glyphLeftOffsets_[c] + rightSample * glyphInfo.maxGlyphWidth_);
            float bottom = coordinateYToVulkan(texts[i].y_ + texts[i].pixelHeight_ - glyphInfo.glyphTopOffsets_[c] + glyphInfo.maxGlyphHeight_ * (bottomSample - topSample) * ASCII_CHAR_COUNT);
            horiAccPx += (glyphInfo.glyphLeftOffsets_[c] + rightSample * glyphInfo.maxGlyphWidth_); // * 1.2; // 1.2 为字间距

            // TODO: 暂时为空格做了个特判
            if (c == ' ') {
                horiAccPx += glyphInfo.maxGlyphWidth_ * 0.5;
            }

            vertexData.push_back(left);
            vertexData.push_back(top);
            vertexData.push_back(leftSample);
            vertexData.push_back(topSample);
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(right);
            vertexData.push_back(top);
            vertexData.push_back(rightSample);
            vertexData.push_back(topSample);
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(right);
            vertexData.push_back(bottom);
            vertexData.push_back(rightSample);
            vertexData.push_back(bottomSample);
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(left);
            vertexData.push_back(bottom);
            vertexData.push_back(leftSample);
            vertexData.push_back(bottomSample);
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);

            indexData.push_back(count + 0);
            indexData.push_back(count + 1);
            indexData.push_back(count + 2);
            indexData.push_back(count + 2);
            indexData.push_back(count + 3);
            indexData.push_back(count + 0);
            count += 4;
        }
    }

    // 创建VkBuffer
    drawResource.vertexBufferInfo_ = vertexBufferManager_->allocBuffer(frameIndex_, sizeof(float) * vertexData.size());
    void *data;
    vkMapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_, 0, sizeof(float) * vertexData.size(),0, &data);
    memcpy(data, vertexData.data(), sizeof(float) * vertexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_);

    // 创建VkBuffer
    drawResource.indexBufferInfo_ = indexBufferManager_->allocBuffer(frameIndex_, sizeof(uint32_t) * indexData.size());
    vkMapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_, 0, sizeof(uint32_t) * indexData.size(),0, &data);
    memcpy(data, indexData.data(), sizeof(uint32_t) * indexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_);

    drawResource.indexCount_ = indexData.size();

    return drawResource;
}

DrawResource Engine2D::drawRRects(std::vector<RRect> &rrects, std::vector<Paint> &paints) {

    // 检查
    if (rrects.size() != paints.size()) {
        LOGE("drawRects inputs do not match!");
    }

    // 该任务生成的绘制资源
    DrawResource drawResource;

    // 创建VkPipeline（如果未曾被创建过）
    if(!pipelineManager_->findPipeline(RRECT_PIPELINE, &drawResource.pipelineInfo_)) {
        // 未曾创建过，则创建
        RRectsDrawTask::createGraphicsPipeline(androidAppCtx_, deviceInfo_->device_,
                                              swapchainInfo_->displaySize_, renderInfo_->renderPass_,
                                              &drawResource.pipelineInfo_);
        pipelineManager_->insertPipeline(RRECT_PIPELINE, drawResource.pipelineInfo_);
    }

    // 生成vertex和index数据
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;

    int base = 0;
    for(int i = 0; i < rrects.size(); i++) {

        float radius = std::min(rrects[i].w_, rrects[i].h_) / 2.0;
        radius = std::min(radius, rrects[i].r_); // 最大可用半径

        /* 先画四个圆角 */
        // 每个扇形的三角形数量
        int triCount = static_cast<int>(radius / 16); // 根据半径决定细分数量
        triCount = triCount > 5 ? triCount : 5; // 每个扇形状细分数量至少为5
        double incRadians = 0.5 * M_PI / triCount;

        // 左上角圆心
        float cornerX = rrects[i].x_ + radius;
        float cornerY = rrects[i].y_ + radius;
        vertexData.push_back(coordinateXToVulkan(cornerX));
        vertexData.push_back(coordinateYToVulkan(cornerY));
        vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        // 细分并插入顶点坐标（左上角）
        double baseRadians =  0.5 * M_PI;
        for (int j = 0; j <= triCount; j++) { // 此处要多一个最终顶点
            double radians = baseRadians + incRadians * j; // 划分角度
            float x = cornerX + radius * static_cast<float>(cos(radians));
            float y = cornerY - radius * static_cast<float>(sin(radians));
            vertexData.push_back(coordinateXToVulkan(x));
            vertexData.push_back(coordinateYToVulkan(y));
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(paints[i].a_);
        }

        // 组织三角形
        for(int j = 0; j < triCount; j++) {
            indexData.push_back(base);
            indexData.push_back(base + j + 1);
            indexData.push_back(base + j + 2);
        }

        base += triCount + 2;


        // 右上角圆心
        cornerX = rrects[i].x_ + rrects[i].w_ - radius;
        cornerY = rrects[i].y_ + radius;
        vertexData.push_back(coordinateXToVulkan(cornerX));
        vertexData.push_back(coordinateYToVulkan(cornerY));
        vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        // 细分并插入顶点坐标（右上角）
        baseRadians =  0;
        for (int j = 0; j <= triCount; j++) { // 此处要多一个最终顶点
            double radians = baseRadians + incRadians * j; // 划分角度
            float x = cornerX + radius * static_cast<float>(cos(radians));
            float y = cornerY - radius * static_cast<float>(sin(radians));
            vertexData.push_back(coordinateXToVulkan(x));
            vertexData.push_back(coordinateYToVulkan(y));
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(paints[i].a_);
        }

        // 组织三角形
        for(int j = 0; j < triCount; j++) {
            indexData.push_back(base);
            indexData.push_back(base + j + 1);
            indexData.push_back(base + j + 2);
        }

        base += triCount + 2;

        // 左下角圆心
        cornerX = rrects[i].x_ + radius;
        cornerY = rrects[i].y_ + rrects[i].h_ - radius;
        vertexData.push_back(coordinateXToVulkan(cornerX));
        vertexData.push_back(coordinateYToVulkan(cornerY));
        vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        // 细分并插入顶点坐标（左上角）
        baseRadians =  1.0 * M_PI;
        for (int j = 0; j <= triCount; j++) { // 此处要多一个最终顶点
            double radians = baseRadians + incRadians * j; // 划分角度
            float x = cornerX + radius * static_cast<float>(cos(radians));
            float y = cornerY - radius * static_cast<float>(sin(radians));
            vertexData.push_back(coordinateXToVulkan(x));
            vertexData.push_back(coordinateYToVulkan(y));
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(paints[i].a_);
        }

        // 组织三角形
        for(int j = 0; j < triCount; j++) {
            indexData.push_back(base);
            indexData.push_back(base + j + 1);
            indexData.push_back(base + j + 2);
        }

        base += triCount + 2;

        // 右下角圆心
        cornerX = rrects[i].x_ + rrects[i].w_ - radius;
        cornerY = rrects[i].y_ + rrects[i].h_ - radius;
        vertexData.push_back(coordinateXToVulkan(cornerX));
        vertexData.push_back(coordinateYToVulkan(cornerY));
        vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        // 细分并插入顶点坐标（左上角）
        baseRadians =  1.5 * M_PI;
        for (int j = 0; j <= triCount; j++) { // 此处要多一个最终顶点
            double radians = baseRadians + incRadians * j; // 划分角度
            float x = cornerX + radius * static_cast<float>(cos(radians));
            float y = cornerY - radius * static_cast<float>(sin(radians));
            vertexData.push_back(coordinateXToVulkan(x));
            vertexData.push_back(coordinateYToVulkan(y));
            vertexData.push_back(paints[i].r_); // TODO: 暂时每个顶点都有颜色值
            vertexData.push_back(paints[i].g_);
            vertexData.push_back(paints[i].b_);
            vertexData.push_back(paints[i].a_);
        }

        // 组织三角形
        for(int j = 0; j < triCount; j++) {
            indexData.push_back(base);
            indexData.push_back(base + j + 1);
            indexData.push_back(base + j + 2);
        }

        base += triCount + 2;

        // 插入三个长方形，从上到下
        // 插入顶点坐标
        float left = coordinateXToVulkan(rrects[i].x_ + radius);
        float top = coordinateYToVulkan(rrects[i].y_);
        float right = coordinateXToVulkan(rrects[i].x_ + rrects[i].w_ - radius);
        float bottom = coordinateYToVulkan(rrects[i].y_ + radius);
        vertexData.push_back(left);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(right);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(right);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(left);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        indexData.push_back(base);
        indexData.push_back(base + 1);
        indexData.push_back(base + 2);
        indexData.push_back(base + 2);
        indexData.push_back(base + 3);
        indexData.push_back(base);

        base += 4;

        // 插入顶点坐标
        left = coordinateXToVulkan(rrects[i].x_);
        top = coordinateYToVulkan(rrects[i].y_ + radius);
        right = coordinateXToVulkan(rrects[i].x_ + rrects[i].w_);
        bottom = coordinateYToVulkan(rrects[i].y_ + rrects[i].h_ - radius);
        vertexData.push_back(left);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(right);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(right);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(left);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        indexData.push_back(base);
        indexData.push_back(base + 1);
        indexData.push_back(base + 2);
        indexData.push_back(base + 2);
        indexData.push_back(base + 3);
        indexData.push_back(base);

        base += 4;

        // 插入顶点坐标
        left = coordinateXToVulkan(rrects[i].x_ + radius);
        top = coordinateYToVulkan(rrects[i].y_ + rrects[i].h_ - radius);
        right = coordinateXToVulkan(rrects[i].x_ + rrects[i].w_ - radius);
        bottom = coordinateYToVulkan(rrects[i].y_ + rrects[i].h_);
        vertexData.push_back(left);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(right);
        vertexData.push_back(top);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(right);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);
        vertexData.push_back(left);
        vertexData.push_back(bottom);
        vertexData.push_back(paints[i].r_);
        vertexData.push_back(paints[i].g_);
        vertexData.push_back(paints[i].b_);
        vertexData.push_back(paints[i].a_);

        indexData.push_back(base);
        indexData.push_back(base + 1);
        indexData.push_back(base + 2);
        indexData.push_back(base + 2);
        indexData.push_back(base + 3);
        indexData.push_back(base);

        base += 4;
    }

    // 创建VkBuffer
    drawResource.vertexBufferInfo_ = vertexBufferManager_->allocBuffer(frameIndex_, sizeof(float) * vertexData.size());
    void *data;
    vkMapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_, 0, sizeof(float) * vertexData.size(),0, &data);
    memcpy(data, vertexData.data(), sizeof(float) * vertexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.vertexBufferInfo_.bufferMemory_);

    // 创建VkBuffer
    drawResource.indexBufferInfo_ = indexBufferManager_->allocBuffer(frameIndex_, sizeof(uint32_t) * indexData.size());
    vkMapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_, 0, sizeof(uint32_t) * indexData.size(),0, &data);
    memcpy(data, indexData.data(), sizeof(uint32_t) * indexData.size());
    vkUnmapMemory(deviceInfo_->device_, drawResource.indexBufferInfo_.bufferMemory_);

    drawResource.indexCount_ = indexData.size();

    return drawResource;
}



float Engine2D::coordinateXToVulkan(float x) {
    return x / static_cast<float>(swapchainInfo_->displaySize_.width) * 2.0f - 1.0f;
}

float Engine2D::coordinateYToVulkan(float y) {
    return y / static_cast<float>(swapchainInfo_->displaySize_.height) * 2.0f - 1.0f;
}