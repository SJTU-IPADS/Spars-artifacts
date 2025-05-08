//
// Created by richardwu on 11/6/24.
//
#include "pipeline_helper.h"
#include "../log.h"

#include <array>

static VkShaderModule loadShaderFromFile(android_app *androidAppCtx, VkDevice device, const char *filePath) {
    // Read the file
    assert(androidAppCtx);
    AAsset *file = AAssetManager_open(androidAppCtx->activity->assetManager,
                                      filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);

    char *fileContent = new char[fileLength];

    AAsset_read(file, fileContent, fileLength);
    AAsset_close(file);

    VkShaderModuleCreateInfo shaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = fileLength,
            .pCode = (const uint32_t *) fileContent,
    };

    VkShaderModule shader;

    CALL_VK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shader));

    delete[] fileContent;

    return shader;
}


/**
  * 创建描述符布局（与管线紧耦合）
  */
VkDescriptorSetLayout createDescriptorSetLayoutImage(VkDevice device)
{
    // 描述绑定
//    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
//    uboLayoutBinding.binding = 0;
//    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 类型
//    uboLayoutBinding.descriptorCount = 1;                                // 数组大小
//    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;            // 在顶点着色器中使用
//    uboLayoutBinding.pImmutableSamplers = nullptr;                       // Optional 图像采样相关描述符

    // 组合图像采样器描述符的绑定
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0; // 不使用ubo，因此这个binding变为第一个了
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

//    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    return descriptorSetLayout;
}


// 创建Graphics Pipeline（使用pipelineCache）专为图片准备
void createGraphicsPipelineHelperImage(android_app *androidAppCtx, VulkanDeviceInfo deviceInfo, VulkanSwapchainInfo swapchainInfo,
                                   VulkanRenderInfo renderInfo, VulkanPipelineInfo *pipelineInfo,
                            char *vsFilePath, char *fsFilePath, VkDescriptorSetLayout descriptorSetLayout) {
//    LOGI("createPipeline %s", vsFilePath);

    memset(pipelineInfo, 0, sizeof(VulkanPipelineInfo));

    VkDevice device = deviceInfo.device_;
    VkExtent2D extent2D = swapchainInfo.displaySize_;
    VkRenderPass renderPass = renderInfo.renderPass_;

    // 管线布局（即定义uniform变量）
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = 1, // 原来是0
            .pSetLayouts = &descriptorSetLayout, // 原来是nullptr
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };

    pipelineInfo->hasDescriptorSetLayout_ = true;
    pipelineInfo->descriptorSetLayout_ = descriptorSetLayout; // 为了销毁

    CALL_VK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                   nullptr, &pipelineInfo->pipelineLayout_));

    VkShaderModule vertexShader = loadShaderFromFile(androidAppCtx, device, vsFilePath);
    VkShaderModule fragmentShader = loadShaderFromFile(androidAppCtx, device, fsFilePath);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            }};

    VkViewport viewports{
            .x = 0,
            .y = 0,
            .width = (float) extent2D.width,
            .height = (float) extent2D.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
            .offset {.x = 0, .y = 0,},
            .extent = extent2D,
    };
    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state // TODO: 为了debug设置一点alpha blending
    VkPipelineColorBlendAttachmentState attachmentStates{
            .blendEnable = VK_TRUE, // 改为VK_TRUE/FALSE即可
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,


    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,

            // TODO: 线框模式和实心模式
             .polygonMode = VK_POLYGON_MODE_FILL,

//            .polygonMode = VK_POLYGON_MODE_LINE, .lineWidth = 1,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    /////////////////////////////////////////////////// TODO: 以下两个需要根据输入设置
    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
            .binding = 0,
            .stride = 4 * sizeof(float), // 二维顶点+二维纹理坐标
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription vertex_input_attributes[2]{{
                                                                         .binding = 0,
                                                                         .location = 0,
                                                                         .format = VK_FORMAT_R32G32_SFLOAT, // 二维顶点
                                                                         .offset = 0,
                                                                 },
                                                                 {
                                                                         .binding = 0,
                                                                         .location = 1,
                                                                         .format = VK_FORMAT_R32G32_SFLOAT, // 二维纹理坐标
                                                                         .offset = 2 * sizeof(float),
                                                                 }};
    ////////////////////////////////////////////////// TODO
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_bindings,
            .vertexAttributeDescriptionCount = 2, // TODO: 这个值也需要根据需求设定
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = pipelineInfo->pipelineLayout_,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    CALL_VK(vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipelineInfo->pipeline_));

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);
}

// 创建Graphics Pipeline（使用pipelineCache）专为图片准备
void createGraphicsPipelineHelperText(android_app *androidAppCtx, VulkanDeviceInfo deviceInfo, VulkanSwapchainInfo swapchainInfo,
                                       VulkanRenderInfo renderInfo, VulkanPipelineInfo *pipelineInfo,
                                       char *vsFilePath, char *fsFilePath, VkDescriptorSetLayout descriptorSetLayout) {
//    LOGI("createPipeline %s", vsFilePath);

    memset(pipelineInfo, 0, sizeof(VulkanPipelineInfo));

    VkDevice device = deviceInfo.device_;
    VkExtent2D extent2D = swapchainInfo.displaySize_;
    VkRenderPass renderPass = renderInfo.renderPass_;

    // 管线布局（即定义uniform变量）
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = 1, // 原来是0
            .pSetLayouts = &descriptorSetLayout, // 原来是nullptr
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };

    pipelineInfo->hasDescriptorSetLayout_ = true;
    pipelineInfo->descriptorSetLayout_ = descriptorSetLayout; // 为了销毁

    CALL_VK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                   nullptr, &pipelineInfo->pipelineLayout_));

    VkShaderModule vertexShader = loadShaderFromFile(androidAppCtx, device, vsFilePath);
    VkShaderModule fragmentShader = loadShaderFromFile(androidAppCtx, device, fsFilePath);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            }};

    VkViewport viewports{
            .x = 0,
            .y = 0,
            .width = (float) extent2D.width,
            .height = (float) extent2D.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
            .offset {.x = 0, .y = 0,},
            .extent = extent2D,
    };
    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state // TODO: 为了debug设置一点alpha blending
    VkPipelineColorBlendAttachmentState attachmentStates{
            .blendEnable = VK_TRUE, // 改为VK_TRUE/FALSE即可
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,


    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,

            // TODO: 线框模式和实心模式
            .polygonMode = VK_POLYGON_MODE_FILL,

//            .polygonMode = VK_POLYGON_MODE_LINE, .lineWidth = 1,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    /////////////////////////////////////////////////// TODO: 以下两个需要根据输入设置
    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
            .binding = 0,
            .stride = 7 * sizeof(float), // 二维顶点+二维纹理坐标+三维颜色
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription vertex_input_attributes[3]{{
                                                                         .binding = 0,
                                                                         .location = 0,
                                                                         .format = VK_FORMAT_R32G32_SFLOAT, // 二维顶点
                                                                         .offset = 0,
                                                                 },
                                                                 {
                                                                         .binding = 0,
                                                                         .location = 1,
                                                                         .format = VK_FORMAT_R32G32_SFLOAT, // 二维纹理坐标
                                                                         .offset = 2 * sizeof(float),
                                                                 },
                                                                 {
                                                                         .binding = 0,
                                                                         .location = 2,
                                                                         .format = VK_FORMAT_R32G32B32_SFLOAT, // 三维颜色
                                                                         .offset = 4 * sizeof(float),
                                                                 }};
    ////////////////////////////////////////////////// TODO
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_bindings,
            .vertexAttributeDescriptionCount = 3, // TODO: 这个值也需要根据需求设定
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = pipelineInfo->pipelineLayout_,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    CALL_VK(vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipelineInfo->pipeline_));

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);
}
// 创建Graphics Pipeline（使用pipelineCache）
void createGraphicsPipelineHelper(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                  VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo,
                                  char *vsFilePath, char *fsFilePath) {

//    LOGI("createPipeline %s", vsFilePath);

    memset(pipelineInfo, 0, sizeof(VulkanPipelineInfo));
    // 管线布局（即定义uniform变量）
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };
    pipelineInfo->hasDescriptorSetLayout_ = false;
    CALL_VK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                   nullptr, &pipelineInfo->pipelineLayout_));

    VkShaderModule vertexShader = loadShaderFromFile(androidAppCtx, device, vsFilePath);
    VkShaderModule fragmentShader = loadShaderFromFile(androidAppCtx, device, fsFilePath);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            }};

    VkViewport viewports{
            .x = 0,
            .y = 0,
            .width = (float) extent2D.width,
            .height = (float) extent2D.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
            .offset {.x = 0, .y = 0,},
            .extent = extent2D,
    };
    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state // TODO: 为了debug设置一点alpha blending
    VkPipelineColorBlendAttachmentState attachmentStates{
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,


    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,

            .polygonMode = VK_POLYGON_MODE_FILL, // TODO

            //.polygonMode = VK_POLYGON_MODE_LINE, .lineWidth = 1,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    /////////////////////////////////////////////////// TODO: 以下两个需要根据输入设置
    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
            .binding = 0,
            .stride = 5 * sizeof(float), // 二维顶点+三维颜色
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription vertex_input_attributes[2]{{
                                                                         .binding = 0,
                                                                         .location = 0,
                                                                         .format = VK_FORMAT_R32G32_SFLOAT, // 二维顶点
                                                                         .offset = 0,
                                                                 },
                                                                 {
                                                                         .binding = 0,
                                                                         .location = 1,
                                                                         .format = VK_FORMAT_R32G32B32_SFLOAT, // 三维颜色
                                                                         .offset = 2 * sizeof(float),
                                                                 }};
    ////////////////////////////////////////////////// TODO
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_bindings,
            .vertexAttributeDescriptionCount = 2, // TODO: 这个值也需要根据需求设定
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = pipelineInfo->pipelineLayout_,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    CALL_VK(vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipelineInfo->pipeline_));

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);
}



// 创建Graphics Pipeline（使用pipelineCache）
void createGraphicsPipelineHelperRRect(android_app *androidAppCtx, VkDevice device, VkExtent2D extent2D,
                                  VkRenderPass renderPass, VulkanPipelineInfo *pipelineInfo,
                                  char *vsFilePath, char *fsFilePath) {

//    LOGI("createPipeline %s", vsFilePath);

    memset(pipelineInfo, 0, sizeof(VulkanPipelineInfo));
    // 管线布局（即定义uniform变量）
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };
    pipelineInfo->hasDescriptorSetLayout_ = false;
    CALL_VK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                   nullptr, &pipelineInfo->pipelineLayout_));

    VkShaderModule vertexShader = loadShaderFromFile(androidAppCtx, device, vsFilePath);
    VkShaderModule fragmentShader = loadShaderFromFile(androidAppCtx, device, fsFilePath);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            }};

    VkViewport viewports{
            .x = 0,
            .y = 0,
            .width = (float) extent2D.width,
            .height = (float) extent2D.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
            .offset {.x = 0, .y = 0,},
            .extent = extent2D,
    };
    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state // TODO: 为了debug设置一点alpha blending
    VkPipelineColorBlendAttachmentState attachmentStates{
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,


    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL, // TODO

            //.polygonMode = VK_POLYGON_MODE_LINE, .lineWidth = 1,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    /////////////////////////////////////////////////// TODO: 以下两个需要根据输入设置
    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
            .binding = 0,
            .stride = 6 * sizeof(float), // 二维顶点+四维颜色
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription vertex_input_attributes[2]{{
                                                                         .binding = 0,
                                                                         .location = 0,
                                                                         .format = VK_FORMAT_R32G32_SFLOAT, // 二维顶点
                                                                         .offset = 0,
                                                                 },
                                                                 {
                                                                         .binding = 0,
                                                                         .location = 1,
                                                                         .format = VK_FORMAT_R32G32B32A32_SFLOAT, // 四维颜色
                                                                         .offset = 2 * sizeof(float),
                                                                 }};
    ////////////////////////////////////////////////// TODO
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_bindings,
            .vertexAttributeDescriptionCount = 2, // TODO: 这个值也需要根据需求设定
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = pipelineInfo->pipelineLayout_,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    CALL_VK(vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &pipelineInfo->pipeline_));

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);
}