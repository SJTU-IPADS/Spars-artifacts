//
// Created by richardwu on 10/30/24.
//

#include "Paint.h"

void Paint::setColor(uint32_t color) {
    uint8_t a = (color & 0xff000000) >> 24;
    uint8_t r = (color & 0x00ff0000) >> 16;
    uint8_t g = (color & 0x0000ff00) >> 8;
    uint8_t b = (color & 0x000000ff);
    r_ = static_cast<float>(r) / 255.0f;
    g_ = static_cast<float>(g) / 255.0f;
    b_ = static_cast<float>(b) / 255.0f;
    a_ = static_cast<float>(a) / 255.0f;
}