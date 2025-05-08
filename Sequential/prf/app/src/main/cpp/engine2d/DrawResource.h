//
// Created by richardwu on 10/31/24.
//

#ifndef PRF_DRAWRESOURCE_H
#define PRF_DRAWRESOURCE_H

#include "BufferManager.h"
#include "PipelineManager.h"
#include "SamplerDescriptorManager.h"
#include "../drawTaskContainer/DrawTaskList.h"

#define MAX_COLLECT_REORDER 5


class DrawTaskList;

/* 一个绘制任务生成的资源（输出） */
class DrawResource {
public:
    uint32_t taskId_;
    VulkanPipelineInfo pipelineInfo_;
    VulkanBufferInfo vertexBufferInfo_;
    VulkanBufferInfo indexBufferInfo_;
    VulkanDescriptorSetInfo descriptorSetInfo_;
    uint32_t indexCount_;

    // 以下方法为插入OrderedPriorityQueue中必须的运算符
    bool operator>(const DrawResource& dr) const;
//    bool operator!=(uint32_t i) const;

    bool canCollect(uint32_t currTaskDone, DrawTaskList *drawTaskList) const;
};


#endif //PRF_DRAWRESOURCE_H
