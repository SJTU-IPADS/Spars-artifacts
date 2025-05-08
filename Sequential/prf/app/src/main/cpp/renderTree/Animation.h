#ifndef Animation_H
#define Animation_H

#include <string>

#include "RenderNode.h"
#include "VSyncInfo.h"

#define NANO_SECOND_PER_SECOND 1000000000L

class RenderNode;

class Animation {
public:
    Animation(RenderNode *target);

    /**
     * 执行该动画，返回是否会导致子节点状态改变
     * @param vsyncInfo
     * @return 是否会导致子节点绝对状态改变
     */
    virtual bool animate(VSyncInfo &vsyncInfo) = 0;
    virtual std::string getName() = 0; // 该动画的名字

protected:
    RenderNode *target_;
};

#endif