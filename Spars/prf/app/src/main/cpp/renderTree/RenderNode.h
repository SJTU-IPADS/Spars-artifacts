#ifndef RenderNode_H
#define RenderNode_H

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <atomic>

#include "DrawCmd.h"
#include "Animation.h"
#include "VSyncInfo.h"

#include "../drawTaskContainer/DrawTaskList.h"

class DrawCmd;
class Animation;

class DrawTaskList;

class RenderNode
{
private:
    /* 树相关函数 */
    RenderNode *parent_ = nullptr;
    std::vector<RenderNode*> children_;
    
    /* 节点属性 */
    uint64_t index_;
    int32_t absX_ = 0, absY_ = 0; // 该渲染节点相对于屏幕的绝对位置，屏幕左上角为0,0
    int32_t absW_ = 0, absH_ = 0; // 该渲染节点的绝对宽高

    int32_t relX_ = 0, relY_ = 0; // 该渲染节点相对于父节点左上角的相对位置
    bool absUpdated_ = false; // 该节点的绝对位置被动画调整过，子节点绝对位置需要更新

    bool visible_ = true; // 对于visible为false的节点，跳过绘制指令

    /* 绘制指令列表 */
    std::vector<std::shared_ptr<DrawCmd>> cmdList_;

public:
    RenderNode();
    ~RenderNode();
    RenderNode(uint64_t index);
    RenderNode(RenderNode *parent, uint64_t index);
    RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH);
    RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH, int32_t relX, int32_t relY);
    RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH, bool visible);
    RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH, bool visible, int32_t relX, int32_t relY);

    /* 树相关函数 */
    void setParent(RenderNode *parent);
    RenderNode *getParent();
    void addChild(RenderNode *child);
    RenderNode *getChild(uint64_t index);
    uint32_t childrenSize();
    std::string dumpTree(uint32_t layer = 0);
    virtual std::string dumpNode();

    /* 节点属性相关函数 */
    int32_t getAbsX();
    int32_t getAbsY();
    int32_t getAbsW();
    int32_t getAbsH();
    int32_t getRelX();
    int32_t getRelY();
    void setAbsX(int32_t absX);
    void setAbsY(int32_t absY);
    void setRelX(int32_t relX);
    void setRelY(int32_t relY);
    void setAbsW(int32_t absW);
    void setAbsH(int32_t absH);
    void setAbsXY(int32_t absX, int32_t absY);
    void setAbsWH(int32_t absW, int32_t absH);
    void setAbsXYWH(int32_t absX, int32_t absY, int32_t absW, int32_t absH);

    bool getVisible();
    void setVisible(bool visible);

    bool getAbsUpdated();
    void setAbsUpdated(bool absUpdated);

    /**
     * 根据本节点的相对信息和父节点的绝对信息，更新本节点的绝对信息
     */
    void updateAbsUsingRel();

    /* 绘制相关函数*/
    void addDrawCmd(std::shared_ptr<DrawCmd> drawCmd);          // 向该节点的cmdList中添加指令
    uint32_t drawCmdCount();
    std::shared_ptr<DrawCmd> getDrawCmd(uint32_t index);
    void clearAllDrawCmd();                     // 清理该节点cmdList所有的指令

};

#endif