//
// Created by richardwu on 10/30/24.
//

#include "Rect.h"
#include <algorithm>

Rect::Rect(float x, float y, float w, float h) {
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;
}

Rect Rect::MakeXYWH(float x, float y, float w, float h) {
    return {x, y, w, h};
}

bool isOverlap(const Rect &a, const Rect &b) {
    // 检查是否存在一个长方形完全位于另一个长方形的外部，再取反
    return !(a.x_ + a.w_ <= b.x_ || // a 在 b 的左侧
             b.x_ + b.w_ <= a.x_ || // b 在 a 的左侧
             a.y_ + a.h_ <= b.y_ || // a 在 b 的上方
             b.y_ + b.h_ <= a.y_);  // b 在 a 的上方
}

Rect getLargerRect(const Rect &a, const Rect &b) {
    // 计算包围矩形的左上角坐标
    float x_min = std::min(a.x_, b.x_);
    float y_min = std::min(a.y_, b.y_);

    // 计算包围矩形的右下角坐标
    float x_max = std::max(a.x_ + a.w_, b.x_ + b.w_);
    float y_max = std::max(a.y_ + a.h_, b.y_ + b.h_);

    // 计算包围矩形的宽和高
    float w = x_max - x_min;
    float h = y_max - y_min;

    // 使用Rect的工厂方法创建包围矩形
    return Rect::MakeXYWH(x_min, y_min, w, h);
}
