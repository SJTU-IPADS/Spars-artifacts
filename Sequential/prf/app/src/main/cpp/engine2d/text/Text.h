//
// Created by richardwu on 11/23/24.
//

#ifndef PRF_TEXT_H
#define PRF_TEXT_H

#include <string>

/* 定义了一个文本，左上角坐标XY，文字大小（字符的高度像素数量），和文字内容*/
class Text {
public:
    float x_, y_;
    float pixelHeight_;
    std::string str_;
    std::string fontPath_;

    static Text MakeText(float x, float y, float pixelHeight, std::string str, std::string fontPath);

    /* GlyphManager中会将Text作为key，查找GlyphInfo */
    bool operator==(const Text& other) const;
private:
    Text(float x, float y, float pixelHeight, std::string str, std::string fontPath);
};


#endif //PRF_TEXT_H
