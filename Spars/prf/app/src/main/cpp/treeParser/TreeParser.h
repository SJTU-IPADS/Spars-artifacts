#ifndef TreeParser_H
#define TreeParser_H

#include "../renderTree/RenderNode.h"
#include "../renderTree/AnimationsList.h"

#include <string>

#include <game-activity/native_app_glue/android_native_app_glue.h>

class TreeParser {

public:
    RenderNode *parse(android_app *androidAppCtx, std::string filename, int32_t &width, int32_t &height, AnimationsList *animationsList);

private:
    uint64_t getIndex(std::string line);
    int32_t getX(std::string line);
    int32_t getY(std::string line);
    int32_t getW(std::string line);
    int32_t getH(std::string line);
    std::string getImagePath(std::string line);
    std::string getTextStr(std::string line);
    int32_t getTextPixelHeight(std::string line);
    std::string getTextFontPath(std::string line);
    uint32_t getColor(std::string line);

    // void getRRectRadii(std::string line, SkVector radii[4]);
    float getRRectR(std::string line);
    bool isSurfaceNode(std::string line);
    bool getVisible(std::string line);

};


#endif