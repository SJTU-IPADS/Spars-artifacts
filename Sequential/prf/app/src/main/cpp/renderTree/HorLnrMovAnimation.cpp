//
// Created by richardwu on 11/25/24.
//

#include "HorLnrMovAnimation.h"

HorLnrMovAnimation::HorLnrMovAnimation(RenderNode *target, int32_t startX, int32_t speed, int32_t endX) : Animation(target)
{
    startX_ = startX;
    speed_ = speed;
    endX_ = endX;
    lastVSyncTS_ = 0;
}

bool HorLnrMovAnimation::animate(VSyncInfo &vsyncInfo)
{
    if(lastVSyncTS_ == 0) {
        currentX_ = startX_;
    } else {
        uint64_t deltaTime = vsyncInfo.vsyncTS - lastVSyncTS_; // in nanosecond
        double distance = ((double)deltaTime) * speed_ / NANO_SECOND_PER_SECOND;
        currentX_ = currentX_ + distance;
        if(speed_ > 0 && currentX_ >= (double)endX_) {
            currentX_ = startX_;
        } else if(speed_ < 0 && currentX_ <= (double)endX_) {
            currentX_ = startX_;
        }
    }
    int32_t newAbsX = static_cast<int32_t>(currentX_);
    target_->setRelX(target_->getRelX() + newAbsX - target_->getAbsX()); // seq只改变相对位置
//    target_->setAbsUpdated(true); // 保证子节点也会一起更新 // 对于sequentialRel不重要
    lastVSyncTS_ = vsyncInfo.vsyncTS;
    return true; // 该动画会改变子节点信息
}

std::string HorLnrMovAnimation::getName()
{
    return "HorLnrMovAnimation";
}