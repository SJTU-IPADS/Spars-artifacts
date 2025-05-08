#include "DrawCmd.h"

std::string getDrawCmdTypeString(DrawCmdType type)
{
#define DRAWCMDTYPESTRING(t) \
    case (t):                \
        return #t
    switch (type)
    {
        DRAWCMDTYPESTRING(RECT_DRAWCMD);
        DRAWCMDTYPESTRING(CIRCLE_DRAWCMD);
        DRAWCMDTYPESTRING(IMAGE_DRAWCMD);
        DRAWCMDTYPESTRING(TEXT_DRAWCMD);
        DRAWCMDTYPESTRING(RRECT_DRAWCMD);
    default:
        return "DRAWCMD_UNKNOWN";
    }
}

DrawCmd::DrawCmd(Paint &paint)
{
    paint_ = paint;
}

Paint DrawCmd::getPaint()
{
    return paint_;
}

std::string DrawCmd::getTypeName()
{
    return getDrawCmdTypeString(getType());
}

// 长方形绘制指令
RectDrawCmd::RectDrawCmd(Paint &paint, Rect &rect) : DrawCmd(paint), rect_(rect)
{
}

DrawCmdType RectDrawCmd::getType()
{
    return DrawCmdType::RECT_DRAWCMD;
}

std::shared_ptr<DrawTask> RectDrawCmd::encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode)
{
    std::vector<Rect> rects;
    std::vector<Paint> paints;
    rects.push_back(Rect::MakeXYWH(renderNode->getAbsX() + rect_.x_,
                                   renderNode->getAbsY() + rect_.y_,
                                   rect_.w_,
                                   rect_.h_));
    paints.push_back(getPaint());

    Rect boundingBox = getAbsoluteBoundingBox(renderNode);
    std::shared_ptr<DrawTask> drawTask(new RectsDrawTask(taskId, boundingBox, rects, paints));
    return std::move(drawTask);
}

Rect RectDrawCmd::getAbsoluteBoundingBox(RenderNode *renderNode) {
    return Rect::MakeXYWH(renderNode->getAbsX() + rect_.x_,
                          renderNode->getAbsY() + rect_.y_,
                          rect_.w_,
                          rect_.h_);
}


// 圆形绘制指令
CircleDrawCmd::CircleDrawCmd(Paint &paint, Circle &circle) : DrawCmd(paint), circle_(circle)
{
}

DrawCmdType CircleDrawCmd::getType()
{
    return DrawCmdType::CIRCLE_DRAWCMD;
}

std::shared_ptr<DrawTask> CircleDrawCmd::encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode)
{
    std::vector<Circle> circles;
    std::vector<Paint> paints;
    circles.push_back(Circle::MakeXYR(renderNode->getAbsX() + circle_.x_,
                                      renderNode->getAbsY() + circle_.y_,
                                      circle_.r_));
    paints.push_back(getPaint());

    Rect boundingBox = getAbsoluteBoundingBox(renderNode);
    std::shared_ptr<DrawTask> drawTask(new CirclesDrawTask(taskId, boundingBox, circles, paints));
    return std::move(drawTask);
}

Rect CircleDrawCmd::getAbsoluteBoundingBox(RenderNode *renderNode) {
    return Rect::MakeXYWH(renderNode->getAbsX() + circle_.x_ - circle_.r_, // 此处x_和y_是圆心，需要转换成左上角坐标
                          renderNode->getAbsY() + circle_.y_ - circle_.r_,
                          circle_.r_ * 2,
                          circle_.r_ * 2);
}


// 图片绘制指令
ImageDrawCmd::ImageDrawCmd(Paint &paint, Image &image) : DrawCmd(paint), image_(image)
{
}

DrawCmdType ImageDrawCmd::getType()
{
    return DrawCmdType::IMAGE_DRAWCMD;
}

std::shared_ptr<DrawTask> ImageDrawCmd::encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode)
{
    Rect rect = Rect::MakeXYWH(renderNode->getAbsX() + image_.rect_.x_,
                               renderNode->getAbsY() + image_.rect_.y_,
                               image_.rect_.w_,
                               image_.rect_.h_);

    Image image = Image::MakeImage(rect, image_.path_);
    Paint paint = getPaint();

    Rect boundingBox = getAbsoluteBoundingBox(renderNode);
    std::shared_ptr<DrawTask> drawTask(new ImageDrawTask(taskId, boundingBox, image, paint));
    return std::move(drawTask);
}

Rect ImageDrawCmd::getAbsoluteBoundingBox(RenderNode *renderNode) {
    return Rect::MakeXYWH(renderNode->getAbsX() + image_.rect_.x_,
                          renderNode->getAbsY() + image_.rect_.y_,
                          image_.rect_.w_,
                          image_.rect_.h_);
}


// 文本绘制指令
TextDrawCmd::TextDrawCmd(Paint &paint, Text &text) : DrawCmd(paint), text_(text)
{
}

DrawCmdType TextDrawCmd::getType()
{
    return DrawCmdType::TEXT_DRAWCMD;
}

std::shared_ptr<DrawTask> TextDrawCmd::encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode)
{
    std::vector<Text> texts;
    std::vector<Paint> paints;
    texts.push_back(Text::MakeText(renderNode->getAbsX() + text_.x_,
                                      renderNode->getAbsY() + text_.y_,
                                      text_.pixelHeight_,
                                      text_.str_,
                                      text_.fontPath_));
    paints.push_back(getPaint());

    Rect boundingBox = getAbsoluteBoundingBox(renderNode);
    std::shared_ptr<DrawTask> drawTask(new TextsDrawTask(taskId, boundingBox, texts, paints));
    return std::move(drawTask);
}

Rect TextDrawCmd::getAbsoluteBoundingBox(RenderNode *renderNode) {

    // LOGI("%s: %f %f %f %f", text_.str_.c_str(), renderNode->getAbsX() + text_.x_, renderNode->getAbsY() + text_.y_, text_.str_.size() * text_.pixelHeight_ * 0.55, text_.pixelHeight_ * 1.25);
    // TODO: 当前只是粗略计算，应该可以每个字符宽度相加
    return Rect::MakeXYWH(renderNode->getAbsX() + text_.x_,
                          renderNode->getAbsY() + text_.y_,
                          text_.str_.size() * text_.pixelHeight_ * 0.55,
                          text_.pixelHeight_ * 1.25);
}

// 圆角长方形绘制指令
RRectDrawCmd::RRectDrawCmd(Paint &paint, RRect &rrect) : DrawCmd(paint), rrect_(rrect)
{
}

DrawCmdType RRectDrawCmd::getType()
{
    return DrawCmdType::RRECT_DRAWCMD;
}

std::shared_ptr<DrawTask> RRectDrawCmd::encapsulateIntoDrawTask(uint32_t taskId, RenderNode *renderNode)
{
    std::vector<RRect> rrects;
    std::vector<Paint> paints;
    rrects.push_back(RRect::MakeXYWHR(renderNode->getAbsX() + rrect_.x_,
                                   renderNode->getAbsY() + rrect_.y_,
                                   rrect_.w_,
                                   rrect_.h_,
                                   rrect_.r_));
    paints.push_back(getPaint());

    Rect boundingBox = getAbsoluteBoundingBox(renderNode);
    std::shared_ptr<DrawTask> drawTask(new RRectsDrawTask(taskId, boundingBox, rrects, paints));
    return std::move(drawTask);
}

Rect RRectDrawCmd::getAbsoluteBoundingBox(RenderNode *renderNode) {
    return Rect::MakeXYWH(renderNode->getAbsX() + rrect_.x_,
                          renderNode->getAbsY() + rrect_.y_,
                          rrect_.w_,
                          rrect_.h_);
}