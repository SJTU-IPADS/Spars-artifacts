//
// Created by richardwu on 10/30/24.
//

#ifndef PRF_PAINT_H
#define PRF_PAINT_H

#include <stdint.h>

/* 储存绘制样式 */
class Paint {
public:
    /* 颜色，介于0.0到1.0之间*/
    float r_ = 0.0f;
    float g_ = 0.0f;
    float b_ = 0.0f;
    float a_ = 0.0f;

    void setColor(uint32_t color);
};


#endif //PRF_PAINT_H
