//
// Created by richardwu on 2024/10/20.
//

#ifndef PRF_RENDER_PASS_H
#define PRF_RENDER_PASS_H

#include <vulkan_wrapper.h>
#include "utils.h"

VkRenderPass getRenderPass(VkDevice device, VkFormat format) {
    VkAttachmentDescription attachmentDescriptions{
            .format = format,
            .samples = VK_SAMPLE_COUNT_1_BIT, // 采样数
            //下面两个会对颜色缓冲和深度缓冲奏效
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // 现有buffer会被用一个颜色清除
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // 渲染内容会被储存起来，以便之后读取
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // 不关心之前的图像，因为会清除
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // 渲染之后的图像会被交换链呈现
    };

    VkAttachmentReference colorReference = {
            .attachment = 0, // index，引用第一个attachment
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    // 描述子流程
    VkSubpassDescription subpassDescription{
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1, // 引用的颜色附着
            .pColorAttachments = &colorReference, // 对应了layout(location=0) out vec4 outColor里的location=0
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
    };

    // 创建渲染流程对象
    VkRenderPassCreateInfo renderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescriptions,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 0,
            .pDependencies = nullptr,
    };

    VkRenderPass renderPass;
    CALL_VK(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr,
                               &renderPass));
    return renderPass;
}

#endif //PRF_RENDER_PASS_H
