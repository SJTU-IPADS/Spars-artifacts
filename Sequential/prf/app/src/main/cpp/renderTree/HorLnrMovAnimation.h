//
// Created by richardwu on 11/25/24.
//

#ifndef PRF_HORLNRMOVANIMATION_H
#define PRF_HORLNRMOVANIMATION_H

#include "Animation.h"

/* 将节点在水平方向循环运动，范围为 [startX, endX) */
class HorLnrMovAnimation : public Animation {
public:
    /* startX: 节点初始的X坐标(包含)
       speed:  节点速度 (向-x方向移动则为负数)，单位为 像素每秒
       endX:   节点最多移动到的X坐标(不包含) */
    HorLnrMovAnimation(RenderNode *target, int32_t startX, int32_t speed, int32_t endX);
    bool animate(VSyncInfo &vsyncInfo) override;
    std::string getName() override;
private:
    int32_t startX_;
    int32_t speed_;
    int32_t endX_;
    uint64_t lastVSyncTS_; // 用于速度计算
    double currentX_;      // 当前位置
};


#endif //PRF_HORLNRMOVANIMATION_H
