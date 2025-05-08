#include "TreeParser.h"
#include "../renderTree/HorLnrMovAnimation.h"

#include "../config.h"

#include <string>
#include <map>

// 将渲染节点的x, y, w, h全部除以2, 以使屏幕可见


RenderNode *TreeParser::parse(android_app *androidAppCtx, std::string filename, int32_t &width,
                              int32_t &height, AnimationsList *animationsList) {
    // 返回值
    RenderNode *rootNode = nullptr;
    width = 0;
    height = 0;

    // 打开文件
    AAsset *file = AAssetManager_open(androidAppCtx->activity->assetManager,
                                      filename.c_str(), AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);

    char *fileContent = new char[fileLength];

    AAsset_read(file, fileContent, fileLength);
    AAsset_close(file);

    // each line of the file.
    char *curLine = fileContent;

    // 读取渲染树文本时，每级深度的节点
    std::map<int, RenderNode *> records;
    // 读取渲染树文本时，每级深度的可见性
    std::map<int, bool> visibilities;


    // read each line from the file
    while (curLine) {

        char * nextLine = strchr(curLine, '\n');
        if (nextLine) *nextLine = '\0';  // temporarily terminate the current line

        std::string line = curLine;

        if (nextLine) *nextLine = '\n';  // then restore newline-char, just to be tidy
        curLine = nextLine ? (nextLine+1) : NULL;

        // 移除注释
        int comment = line.find("//");
        if (comment != -1) {
            line = line.substr(0, comment);
        }

        int i = line.find("|");

        // std::cout << i << std::endl;

        // 该行不为渲染节点，跳过
        if (i == -1) {
            continue;
        }
            // 根节点
        else if (i == 0) {
            // 该节点在RS中不包含任何东西，完全是占位作用
            rootNode = new RenderNode(nullptr, getIndex(line), getX(line), getY(line),
                                      getW(line), getH(line));
            records[i] = rootNode;
            visibilities[i] = true;
        }
            // 其他节点
        else {
            RenderNode *parent = records[i - 2]; // 存有的父节点
            // 创建节点（将相对坐标设置为绝对坐标）
            RenderNode *curr = new RenderNode(parent, getIndex(line),
                                              parent->getAbsX() + getX(line),
                                              parent->getAbsY() + getY(line), getW(line),
                                              getH(line),
                                              getX(line),
                                              getY(line));
            parent->addChild(curr);
            records[i] = curr;

            // 对于宽高小于0的情况
            if (curr->getAbsW() < 0) {
                curr->setAbsX(curr->getAbsX() + curr->getAbsW());
                curr->setAbsW(0 - curr->getAbsW());
            }
            if (curr->getAbsH() < 0) {
                curr->setAbsY(curr->getAbsY() + curr->getAbsH());
                curr->setAbsH(0 - curr->getAbsH());
            }

            // // 查验正确性
            // if (curr->getAbsX() < 0 || curr->getAbsY() < 0 || curr->getAbsW() < 0 || curr->getAbsH() < 0) {
            //     throw std::runtime_error("Invalid Render Node with negative value!");
            // }

            // 获得屏幕宽高
            if (line.find("DISPLAY_NODE") != -1) {
                if (width != 0) {
                    throw std::runtime_error("failed to handle multiple display nodes!");
                }
                width = curr->getAbsX() + curr->getAbsW();
                height = curr->getAbsY() + curr->getAbsH();
            }

            // 插入绘制指令和动画
            if (curr->getAbsW() > 0 && curr->getAbsH() > 0) {

                    // 绘制图片
                    if (line.find("Image:") != -1) {
                        Paint paint;
                        Rect rect = Rect::MakeXYWH(0, 0, curr->getAbsW(), curr->getAbsH());
                        Image image = Image::MakeImage(rect, getImagePath(line)); // 此图像未被真正解码
                        auto cmd = std::make_shared<ImageDrawCmd>(paint, image);
                        curr->addDrawCmd(cmd);
                    }
                    // 绘制文本
                    else if (line.find("Text:") != -1) {
                        Paint paint;
                        paint.setColor(getColor(line));
                        Text text = Text::MakeText(0, 0, getTextPixelHeight(line), getTextStr(line), getTextFontPath(line)); // TODO: 将pixelHeight变为可设置的
                        auto cmd = std::make_shared<TextDrawCmd>(paint, text);
                        curr->addDrawCmd(cmd);
                    }
                    // 绘制圆角矩形
                    else if (line.find("CornerRadius") != -1) {
                        float r = getRRectR(line);

                        Paint paint;
                        paint.setColor(getColor(line));
                        RRect rrect = RRect::MakeXYWHR(0, 0, curr->getAbsW(), curr->getAbsH(), r);
                        auto cmd = std::make_shared<RRectDrawCmd>(paint, rrect);
                        curr->addDrawCmd(cmd);

                    }
                    // 绘制矩形
                    else if (line.find("Rect") != -1)
                    {
                        Paint paint;
                        paint.setColor(getColor(line));
                        Rect rect = Rect::MakeXYWH(0, 0, curr->getAbsW(), curr->getAbsH());
                        auto cmd = std::make_shared<RectDrawCmd>(paint, rect);
                        curr->addDrawCmd(cmd);
                    }
                    // 绘制圆形
                    else if (line.find("Circle") != -1)
                    {
                        Paint paint;
                        paint.setColor(getColor(line));
                        Circle circle = Circle::MakeXYR(curr->getAbsW()/2, curr->getAbsH()/2, std::min(curr->getAbsW(), curr->getAbsH())/2);
                        auto cmd = std::make_shared<CircleDrawCmd>(paint, circle);
                        curr->addDrawCmd(cmd);
                    }



                    // 插入动画
                    if (line.find("Anim: ") != -1) {
                        if (line.find("HorLnrMov") != -1) {
                            auto anim = std::make_shared<HorLnrMovAnimation>(curr, curr->getAbsX(), -100, curr->getAbsX() - 700);
                            animationsList->addAnimation(anim);
                        }
                    }
            }

            // 标记可见性
            if (isSurfaceNode(line)) { // 对于surface节点，使用自己的可见性
                visibilities[i] = getVisible(line);
            } else { // 对于其他节点，使用父节点的可见性
                visibilities[i] = visibilities[i - 2];
            }
            curr->setVisible(visibilities[i]);
        }
    }

    delete[] fileContent;
    return rootNode;
}

static uint64_t index_ = 0;
uint64_t TreeParser::getIndex(std::string line) {
    int start = line.find("[");
    int end = line.find("]");
    // return stol(line.substr(start + 1, end - start - 1));
    return index_++;// ignore specified index
}

int32_t TreeParser::getX(std::string line) {
    int start = line.find("Bounds[");
    int end = line.find(" ", start);
    try {
        // std::cout << line.substr(start + 7, end - start - 7) << std::endl;
        return stoi(line.substr(start + 7, end - start - 7)) / DIVIDE_BY;
    } catch (std::invalid_argument const &ex) {
        // ignore invalid arguments like -inf
        // throw std::runtime_error("failed to get parameter: " + line.substr(start + 7, end - start - 7));
    }
    return 0;
}

int32_t TreeParser::getY(std::string line) {
    int base = line.find("Bounds[");
    int start = line.find(" ", base);
    int end = line.find(" ", start + 1);
    try {
        // std::cout << line.substr(start + 1, end - start - 1) << std::endl;
        return stoi(line.substr(start + 1, end - start - 1)) / DIVIDE_BY;
    } catch (std::invalid_argument const &ex) {
        // ignore invalid arguments like -inf
        // throw std::runtime_error("failed to get parameter: " + line.substr(start + 7, end - start - 7));
    }
    return 0;
}

int32_t TreeParser::getW(std::string line) {
    int base = line.find("Bounds[");
    int base2 = line.find(" ", base);
    int start = line.find(" ", base2 + 1);
    int end = line.find(" ", start + 1);
    try {
        // std::cout << line.substr(start + 1, end - start - 1) << std::endl;
        return stoi(line.substr(start + 1, end - start - 1)) / DIVIDE_BY;
    } catch (std::invalid_argument const &ex) {
        // ignore invalid arguments like -inf
        // throw std::runtime_error("failed to get parameter: " + line.substr(start + 7, end - start - 7));
    }
    return 0;
}

int32_t TreeParser::getH(std::string line) {
    int base = line.find("Bounds[");
    int base2 = line.find(" ", base);
    int base3 = line.find(" ", base2 + 1);
    int start = line.find(" ", base3 + 1);
    int end = line.find("]", start + 1);
    try {
        // std::cout << line.substr(start + 1, end - start - 1) << std::endl;
        return stoi(line.substr(start + 1, end - start - 1)) / DIVIDE_BY;
    } catch (std::invalid_argument const &ex) {
        // ignore invalid arguments like -inf
        // throw std::runtime_error("failed to get parameter: " + line.substr(start + 7, end - start - 7));
    }
    return 0;
}

std::string TreeParser::getImagePath(std::string line) {
    int base = line.find("Image:");
    int start = line.find("\"", base);
    int end = line.find("\"", start + 1);
    return std::string("Texture/") + line.substr(start + 1, end - start - 1);
}

std::string TreeParser::getTextStr(std::string line) {
    int base = line.find("Text:");
    int start = line.find("\"", base);
    int end = line.find("\"", start + 1);
    return line.substr(start + 1, end - start - 1);
}

std::string TreeParser::getTextFontPath(std::string line) {
    int base = line.find("Text:");
    int base2 = line.find("\"", base);
    int base3 = line.find("\"", base2 + 1);
    int start = line.find("\"", base3 + 1);
    int end = line.find("\"", start + 1);
    return std::string("/system/fonts/") + line.substr(start + 1, end - start - 1);
}

int32_t TreeParser::getTextPixelHeight(std::string line) {
    int base = line.find("Text:");
    int base2 = line.find("\"", base);
    int base3 = line.find("\"", base2 + 1);
    int base4 = line.find("\"", base3 + 1);
    int start = line.find("\"", base4 + 1);
    int end = line.find("]", start + 1);
    try {
        // std::cout << line.substr(start + 1, end - start - 1) << std::endl;
        return stoi(line.substr(start + 1, end - start - 1)) / DIVIDE_BY;
    } catch (std::invalid_argument const &ex) {
        // ignore invalid arguments like -inf
        // throw std::runtime_error("failed to get parameter: " + line.substr(start + 7, end - start - 7));
    }
    return 0;
}

uint32_t TreeParser::getColor(std::string line) {
    int base = line.find("Paint:");
    if (base == -1) {
        return rand();
    }
    int start = line.find("[", base);
    int end = line.find("]", start + 1);
    try {
        // std::cout << line.substr(start + 1, end - start - 1) << std::endl;
        return stoul(line.substr(start + 1, end), nullptr, 16);
    } catch (std::invalid_argument const &ex) {
        throw std::runtime_error("failed to get color: " + line.substr(start + 1, end));
    }
}

// TODO: 支持复杂自定义圆角
//void TreeParser::getRRectRadii(std::string line, SkVector radii[4])
//{
//    int base = line.find("CornerRadius");
//    int start1 = line.find("[", base);
//    int end1 = line.find(" ", start1 + 1);
//    int end2 = line.find(" ", end1 + 1);
//    int end3 = line.find(" ", end2 + 1);
//    int end4 = line.find("]", end3+ 1);
//
//    try {
//        float f1 = stof(line.substr(start1 + 1, end1 - start1 - 1));
//        float f2 = stof(line.substr(end1 + 1, end2 - end1 - 1));
//        float f3 = stof(line.substr(end2 + 1, end3 - end2 - 1));
//        float f4 = stof(line.substr(end3 + 1, end4 - end3 - 1));
//        radii[0] = {f1, f1};
//        radii[1] = {f2, f2};
//        radii[2] = {f3, f3};
//        radii[3] = {f4, f4};
//    } catch (std::invalid_argument const& ex) {
//        // ignore invalid arguments like -inf
//        throw std::runtime_error("failed to get rrect radii!");
//    }
//}

float TreeParser::getRRectR(std::string line)
{
    int base = line.find("CornerRadius");
    int start1 = line.find("[", base);
    int end1 = line.find(" ", start1 + 1);

    try {
        float f1 = stof(line.substr(start1 + 1, end1 - start1 - 1)) / DIVIDE_BY;
        return f1;
    } catch (std::invalid_argument const& ex) {
        // ignore invalid arguments like -inf
        throw std::runtime_error("failed to get rrect radii!");
    }
}

bool TreeParser::getVisible(std::string line) {
    return line.find("Region [Empty]") == -1;
}

bool TreeParser::isSurfaceNode(std::string line) {
    return line.find("SURFACE_NODE") != -1;
}