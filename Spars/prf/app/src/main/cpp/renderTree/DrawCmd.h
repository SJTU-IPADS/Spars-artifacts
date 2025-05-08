#ifndef DrawCmd_H
#define DrawCmd_H

#include <string>

#include "../engine2d/paint/Paint.h"
#include "../engine2d/rects/Rect.h"
#include "../engine2d/circles/Circle.h"
#include "../engine2d/image/Image.h"
#include "../engine2d/text/Text.h"
#include "../engine2d/rrects/RRect.h"
#include "../engine2d/DrawTask.h"
#include "RenderNode.h"

class RenderNode;
class DrawTask;

enum DrawCmdType : uint16_t
{
    RECT_DRAWCMD,
    CIRCLE_DRAWCMD,
    IMAGE_DRAWCMD,
    TEXT_DRAWCMD,
    RRECT_DRAWCMD,
};

std::string getDrawCmdTypeString(DrawCmdType type);

class DrawCmd
{
public:
    DrawCmd(Paint &paint);
    Paint getPaint();
    std::string getTypeName();

    // 该DrawCmd的绘制范围，为了合批和乱序插入

    /* 不同种类的cmd需要重写的函数 */
    virtual DrawCmdType getType() = 0;
    virtual std::shared_ptr<DrawTask> encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode) = 0;
    virtual Rect getAbsoluteBoundingBox(RenderNode *renderNode) = 0; // 得到该DrawCmd的绝对绘制范围，为了合批和乱序插入

private:
    Paint paint_;
};

/* 该指令绘制长方形 */
class RectDrawCmd : public DrawCmd
{
public:
    RectDrawCmd(Paint &paint, Rect &rect);
    DrawCmdType getType() override;
    std::shared_ptr<DrawTask> encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode) override;
    Rect getAbsoluteBoundingBox(RenderNode *renderNode) override;

// private: // TODO: for convenience
    Rect rect_;
};

/* 该指令绘制圆形 */
class CircleDrawCmd : public DrawCmd
{
public:
    CircleDrawCmd(Paint &paint, Circle &circle);
    DrawCmdType getType() override;
    std::shared_ptr<DrawTask> encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode) override;
    Rect getAbsoluteBoundingBox(RenderNode *renderNode) override;

// private: // TODO: for convenience
    Circle circle_;
};

/* 该指令绘制图片 */
class ImageDrawCmd : public DrawCmd
{
public:
    ImageDrawCmd(Paint &paint, Image &image);
    DrawCmdType getType() override;
    std::shared_ptr<DrawTask> encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode) override;
    Rect getAbsoluteBoundingBox(RenderNode *renderNode) override;

// private: // TODO: for convenience
    Image image_;
};

/* 该指令绘制文字 */
class TextDrawCmd : public DrawCmd
{
public:
    TextDrawCmd(Paint &paint, Text &text);
    DrawCmdType getType() override;
    std::shared_ptr<DrawTask> encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode) override;
    Rect getAbsoluteBoundingBox(RenderNode *renderNode) override;

// private: // TODO: for convenience
    Text text_;
};

/* 该指令绘制圆角长方形 */
class RRectDrawCmd : public DrawCmd
{
public:
    RRectDrawCmd(Paint &paint, RRect &rrect);
    DrawCmdType getType() override;
    std::shared_ptr<DrawTask> encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode) override;
    Rect getAbsoluteBoundingBox(RenderNode *renderNode) override;

// private: // TODO: for convenience
    RRect rrect_;
};

#endif