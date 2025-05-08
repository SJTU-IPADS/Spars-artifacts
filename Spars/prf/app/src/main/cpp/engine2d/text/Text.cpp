//
// Created by richardwu on 11/23/24.
//

#include "Text.h"

#include <utility>
#include "../../log.h"

Text::Text(float x, float y, float pixelHeight, std::string str, std::string fontPath) : x_(x), y_(y), pixelHeight_(pixelHeight), str_(std::move(str)), fontPath_(std::move(fontPath)) {
}

Text Text::MakeText(float x, float y, float pixelHeight, std::string str, std::string fontPath) {
    return {x, y, pixelHeight, std::move(str), std::move(fontPath)};
}

bool Text::operator==(const Text& other) const {

    // 比较字体和大小
    return fontPath_ == other.fontPath_ && pixelHeight_ == other.pixelHeight_;
}