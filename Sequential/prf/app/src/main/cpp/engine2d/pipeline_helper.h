//
// Created by richardwu on 10/21/24.
//

#ifndef PRF_PIPELINE_HELPER_H
#define PRF_PIPELINE_HELPER_H

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan_wrapper.h>

#include "PipelineManager.h"
#include "../vulkan/utils.h"
#include "image/Image.h"

// 绘制矩形和圆形的管线
void createGraphicsPipelineHelper(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                            VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo,
                            char *vsFilePath, char *fsFilePath);

void createGraphicsPipelineHelperRRect(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                  VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo,
                                  char *vsFilePath, char *fsFilePath);

// 绘制图片的管线
void createGraphicsPipelineHelperImage(android_app *androidAppCtx, VulkanDeviceInfo deviceInfo, VulkanSwapchainInfo swapchainInfo,
                                                           VulkanRenderInfo renderInfo, VulkanPipelineInfo *pipelineInfo,
                                                           char *vsFilePath, char *fsFilePath, VkDescriptorSetLayout descriptorSetLayout);

// 绘制图片的管线
void createGraphicsPipelineHelperText(android_app *androidAppCtx, VulkanDeviceInfo deviceInfo, VulkanSwapchainInfo swapchainInfo,
                                       VulkanRenderInfo renderInfo, VulkanPipelineInfo *pipelineInfo,
                                       char *vsFilePath, char *fsFilePath, VkDescriptorSetLayout descriptorSetLayout);

// 绘制图片所用的descriptor（用来存放纹理）
VkDescriptorSetLayout createDescriptorSetLayoutImage(VkDevice device);


#endif //PRF_PIPELINE_HELPER_H
