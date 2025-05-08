//
// Created by richardwu on 11/25/24.
//

#include "AnimationsList.h"

bool AnimationsList::animateAll(VSyncInfo &vsyncInfo) {
    bool ret = false;
    for(auto anim : animList_) {
        ret |= anim->animate(vsyncInfo); // 有一个动画需要子节点状态改变就会设置为true
    }
    return ret;
}

void AnimationsList::addAnimation(const std::shared_ptr<Animation>& animation) {
    animList_.push_back(animation);
}

void AnimationsList::updateTree(RenderNode *root, bool needUpdate) {

    // 该节点需要更新，且该节点的子节点也会更新
    if (needUpdate) {
        root->updateAbsUsingRel();
    }

    if (root->getAbsUpdated()) {
        needUpdate = true;
        root->setAbsUpdated(false); // 置回false
    }

    // 递归
    for (int i = 0; i < root->childrenSize(); i++)
    {
        updateTree(root->getChild(i), needUpdate);
    }
}