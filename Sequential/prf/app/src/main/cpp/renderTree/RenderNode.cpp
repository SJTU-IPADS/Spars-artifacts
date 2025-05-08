#include "RenderNode.h"

RenderNode::RenderNode() {
}

RenderNode::~RenderNode() {
}

RenderNode::RenderNode(uint64_t index)
{
    index_ = index;
}

RenderNode::RenderNode(RenderNode *parent, uint64_t index)
{
    parent_ = parent;
    index_ = index;
}

RenderNode::RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH)
{
    parent_ = parent;
    index_ = index;
    setAbsXYWH(absX, absY, absW, absH);
}

RenderNode::RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH, int32_t relX, int32_t relY)
{
    parent_ = parent;
    index_ = index;
    setAbsXYWH(absX, absY, absW, absH);
    relX_ = relX;
    relY_ = relY;
}

RenderNode::RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH, bool visible)
{
    parent_ = parent;
    index_ = index;
    setAbsXYWH(absX, absY, absW, absH);
    visible_ = visible;
}

RenderNode::RenderNode(RenderNode *parent, uint64_t index, int32_t absX, int32_t absY, int32_t absW, int32_t absH, bool visible, int32_t relX, int32_t relY)
{
    parent_ = parent;
    index_ = index;
    setAbsXYWH(absX, absY, absW, absH);
    visible_ = visible;
    relX_ = relX;
    relY_ = relY;
}

void RenderNode::setParent(RenderNode *parent)
{
    parent_ = parent;
}

RenderNode *RenderNode::getParent()
{
    return parent_;
}

RenderNode *RenderNode::getChild(uint64_t index)
{
    return children_[index];
}

void RenderNode::addChild(RenderNode *child)
{
    children_.push_back(child);
}

uint32_t RenderNode::childrenSize()
{
    return children_.size();
}

std::string RenderNode::dumpNode()
{
    std::stringstream ss;
    ss << "[" << index_ << "] " << "v" << visible_ << ", x" << absX_ << " y" << absY_ << " w" << absW_ << " h" << absH_ << ",";

    for(std::shared_ptr<DrawCmd> drawCmd : cmdList_) {
        ss << " " << drawCmd->getTypeName();
    }

    return ss.str();
}

std::string RenderNode::dumpTree(uint32_t layer)
{
    std::stringstream ss;
    
    for (int i = 0; i < layer; i++)
    {
        ss << "  ";
    }
    ss << dumpNode();
    ss << "\n";
    for (RenderNode *child : children_)
    {
        ss << child->dumpTree(layer + 1);
    }

    return ss.str();
}

int32_t RenderNode::getAbsX() {
    return absX_;
}

int32_t RenderNode::getAbsY() {
    return absY_;
}

int32_t RenderNode::getRelX() {
    return relX_;
}

int32_t RenderNode::getRelY() {
    return relY_;
}


int32_t RenderNode::getAbsW() {
    return absW_;
}

int32_t RenderNode::getAbsH() {
    return absH_;
}

void RenderNode::setAbsX(int32_t absX) {
    absX_ = absX;
}

void RenderNode::setAbsY(int32_t absY) {
    absY_ = absY;
}

void RenderNode::setRelX(int32_t relX) {
    relX_ = relX;
}

void RenderNode::setRelY(int32_t relY) {
    relY_ = relY;
}

void RenderNode::setAbsW(int32_t absW) {
    absW_ = absW;
}

void RenderNode::setAbsH(int32_t absH) {
    absH_ = absH;
}

void RenderNode::setAbsXY(int32_t absX, int32_t absY) {
    absX_ = absX;
    absY_ = absY;
}

void RenderNode::setAbsWH(int32_t absW, int32_t absH) {
    absW_ = absW;
    absH_ = absH;
}

void RenderNode::setAbsXYWH(int32_t absX, int32_t absY, int32_t absW, int32_t absH) {
    setAbsXY(absX, absY);
    setAbsWH(absW, absH);
}

bool RenderNode::getVisible() {
    return visible_;
}

void RenderNode::setVisible(bool visible) {
    visible_ = visible;
}

bool RenderNode::getAbsUpdated() {
    return absUpdated_;
}

void RenderNode::setAbsUpdated(bool absUpdated) {
    absUpdated_ = absUpdated;
}

void RenderNode::updateAbsUsingRel() {
    if (parent_ != nullptr) {
        absX_ = parent_->absX_ + relX_;
        absY_ = parent_->absY_ + relY_;
    }
}

void RenderNode::addDrawCmd(std::shared_ptr<DrawCmd> drawCmd) {
    cmdList_.push_back(drawCmd);
}

uint32_t RenderNode::drawCmdCount() {
    return cmdList_.size();
}

std::shared_ptr<DrawCmd> RenderNode::getDrawCmd(uint32_t index) {
    return cmdList_[index];
}

void RenderNode::clearAllDrawCmd() {
    cmdList_.clear();
}