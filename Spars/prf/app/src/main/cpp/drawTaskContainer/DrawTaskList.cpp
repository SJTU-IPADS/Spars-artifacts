//
// Created by richardwu on 11/7/24.
//

#include "DrawTaskList.h"

uint32_t DrawTaskList::getTaskNum() {
    return drawTasks_.size();
}

std::shared_ptr<DrawTask> DrawTaskList::getDrawTask(uint32_t index) {
    return drawTasks_[index];
}


void DrawTaskList::generateFromRenderTree(RenderNode *rootNode) {
    drawTasks_.clear();

//    //////////////////////////////////////////////////////////////////////////// TODO: 测试代码，待删除
//    for(uint32_t i = 0; i < 10; i++) {
//        if (i % 2 == 0) {
//            std::vector<Rect> rects;
//            std::vector<Paint> paints;
//            for (uint32_t j = 0; j < 10; j++) {
//                rects.push_back(
//                        Rect::MakeXYWH(rand() % 1500, rand() % 3000, rand() % 100, rand() % 100));
//                Paint p;
//                p.setColor(rand());
//                paints.push_back(p);
//            }
//            std::unique_ptr<RectsDrawTask> drawTask(new RectsDrawTask(i, rects, paints));
//
//            drawTasks_.push_back(std::move(drawTask));
//        } else if(i % 2 == 1) {
//            std::vector<Circle> circles;
//            std::vector<Paint> paints;
//            for (uint32_t j = 0; j < 10; j++) {
//                circles.push_back(
//                        Circle::MakeXYR(rand() % 1500, rand() % 3000, rand() % 50));
//                Paint p;
//                p.setColor(rand());
//                paints.push_back(p);
//            }
//            std::unique_ptr<CirclesDrawTask> drawTask(new CirclesDrawTask(i, circles, paints));
//
//            drawTasks_.push_back(std::move(drawTask));
//        }
//    }
//    //////////////////////////////////////////////////////////////////////////// TODO: 测试代码，待删除

    drawRenderTree(rootNode);
}

void DrawTaskList::drawRenderTree(RenderNode *rootNode) {
    drawRenderNode(rootNode);

    for(int i = 0; i < rootNode->childrenSize(); i++) {
        drawRenderTree(rootNode->getChild(i));
    }
}

void DrawTaskList::drawRenderNode(RenderNode *renderNode) {

    // 跳过不可见节点
    if (!renderNode->getVisible()) {
        return;
    }

    uint32_t drawCmdCount = renderNode->drawCmdCount();

    // 依次处理所有drawCmd
    for (int i = 0; i < drawCmdCount; i++) {

        auto drawCmd = renderNode->getDrawCmd(i);

        // 首先试图合批，若未成功合批，则将该DrawCmd单独封装成DrawTask插在末尾
        if (!batchDrawCmdWithDrawTasks(drawCmd.get(), renderNode)) {
            uint32_t taskId = drawTasks_.size(); // 确保插入id始终从0开始递增
            drawTasks_.push_back(drawCmd->encapsulateIntoDrawTask(taskId, renderNode));
        }

    }
}

bool DrawTaskList::batchDrawCmdWithDrawTasks(DrawCmd *drawCmd, RenderNode *renderNode) {

    // 合批
    for(int i = 0; i < MAX_BATCH_ITERATION; i++) {

        int size = static_cast<int>(drawTasks_.size());
        // 已经迭代到头了
        if ((size - 1 - i) < 0) {
            return false;
        }

        Rect cmdBoundBox = drawCmd->getAbsoluteBoundingBox(renderNode);
        uint32_t ret = drawTasks_[size - 1 - i]->batchWith(drawCmd, cmdBoundBox, renderNode);

        if (ret == BATCH_SUCCESSFUL) {
            return true;
        } else if (ret == BATCH_FAIL_OVERLAP) {
            // 无法再往前迭代了
            return false;
        }
    }

    return false;
}
