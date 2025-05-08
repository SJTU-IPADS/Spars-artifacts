//
// Created by richardwu on 11/27/24.
//

#include "RRect.h"

RRect::RRect(float x, float y, float w, float h, float r) {
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;
    r_ = r;
}

RRect RRect::MakeXYWHR(float x, float y, float w, float h, float r) {
    return {x, y, w, h, r};
}