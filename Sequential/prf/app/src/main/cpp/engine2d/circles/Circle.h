//
// Created by richardwu on 11/6/24.
//

#ifndef PRF_CIRCLE_H
#define PRF_CIRCLE_H


/* 定义了一个圆形，以圆心坐标和半径定义*/
class Circle {
public:
    float x_, y_, r_;

    static Circle MakeXYR(float x, float y, float r);

private:
    Circle(float x, float y, float r);
};


#endif //PRF_CIRCLE_H
