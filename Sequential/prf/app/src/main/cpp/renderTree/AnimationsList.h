//
// Created by richardwu on 11/25/24.
//

#ifndef PRF_ANIMATIONSLIST_H
#define PRF_ANIMATIONSLIST_H

#include "VSyncInfo.h"
#include "Animation.h"

/**
 * 管理系统中所有的动画 TODO:与生命周期
 */
class AnimationsList {
public:
    /**
     * 执行列表中的所有动画，动画只会更新动画节点的absUpdated，并不会更新所有子节点的绝对位置
     * 因此在执行完animateAll后，可能需要执行updateTree
     * @param vsyncInfo
     * @return 是否有必要执行updateTree
     */
    bool animateAll(VSyncInfo &vsyncInfo);

    /**
     * 更新相对位置到绝对位置，对于sequentialRel branch，所有节点都需要计算
     * @param root 以该节点为root的子树
     */
    void updateTree(RenderNode *root); // 更新所有节点的绝对信息
    void addAnimation(const std::shared_ptr<Animation>& animation);

private:
    std::vector<std::shared_ptr<Animation>> animList_; // 系统中所有的动画
};


#endif //PRF_ANIMATIONSLIST_H
