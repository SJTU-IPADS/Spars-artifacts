//
// Created by richardwu on 10/30/24.
//

#ifndef PRF_RECT_H
#define PRF_RECT_H

/* 定义了一个长方形，以左上角坐标XY和长宽WH定义*/
class Rect {
public:
    float x_, y_, w_, h_;

    static Rect MakeXYWH(float x, float y, float w, float h);

private:
    Rect(float x, float y, float w, float h);
};

/* 以下为常用的工具函数 */
/**
 * 判断两个长方形是否重叠
 * @param a
 * @param b
 * @return
 */
bool isOverlap(const Rect &a, const Rect &b);

/**
 * 返回一个包含a和b的更大的包围矩形
 * @param a
 * @param b
 * @return
 */
Rect getLargerRect(const Rect &a, const Rect &b);

#endif //PRF_RECT_H
