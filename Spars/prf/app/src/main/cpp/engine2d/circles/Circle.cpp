//
// Created by richardwu on 11/6/24.
//

#include "Circle.h"

Circle::Circle(float x, float y, float r) {
    x_ = x;
    y_ = y;
    r_ = r;
}

Circle Circle::MakeXYR(float x, float y, float r) {
    return {x, y, r};
}