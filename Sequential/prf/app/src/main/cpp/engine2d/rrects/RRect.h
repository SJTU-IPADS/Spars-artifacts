//
// Created by richardwu on 11/27/24.
//

#ifndef PRF_RRECT_H
#define PRF_RRECT_H

/**
 * 圆角矩形，r为角的半径
 */
class RRect {
public:
    float x_, y_, w_, h_, r_;

    static RRect MakeXYWHR(float x, float y, float w, float h, float r);

private:
    RRect(float x, float y, float w, float h, float r);
};


#endif //PRF_RRECT_H
