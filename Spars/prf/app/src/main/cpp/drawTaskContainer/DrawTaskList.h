//
// Created by richardwu on 11/7/24.
//

#ifndef PRF_DRAWTASKLIST_H
#define PRF_DRAWTASKLIST_H

#include "../engine2d/DrawTask.h"
#include "../renderTree/DrawCmd.h"
#include "../renderTree/RenderNode.h"

#include <memory>

#define MAX_BATCH_ITERATION 5 // 合批最多向前迭代的次数

class DrawCmd;
class DrawTask;
class RenderNode;

/**
 * 合批完成后，所有的DrawTask，管理他们的生命周期
 * 由RenderTree遍历生成，再通过RenderWorkerPool分发给所有工作线程
 */
class DrawTaskList {
public:

    uint32_t getTaskNum();
    std::shared_ptr<DrawTask> getDrawTask(uint32_t index);

    /**
     * 将一棵渲染树（合批）生成drawTasks_
     * 之前的drawTasks_会被清空
     * @param rootNode 渲染树的根节点
     */
    void generateFromRenderTree(RenderNode *rootNode);

private:

    std::vector<std::shared_ptr<DrawTask>> drawTasks_;

    /**
     * 将一棵渲染树（合批）生成drawTasks_
     * @param rootNode 渲染树的根节点
     */
    void drawRenderTree(RenderNode *rootNode);

    /**
     * 将一个渲染节点中的DrawCmd加入到drawTasks_
     * @param renderNode
     */
    void drawRenderNode(RenderNode *renderNode);

    /**
     * 将新的DrawCmd与drawTasks_进行合批
     * @return 是否成功合批
     */
    bool batchDrawCmdWithDrawTasks(DrawCmd *drawCmd, RenderNode *renderNode);
};


#endif //PRF_DRAWTASKLIST_H
