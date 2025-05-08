//
// Created by richardwu on 11/15/24.
//

#include "Image.h"

Image::Image(Rect rect, std::string path) : rect_(rect), path_(path) {
}

Image Image::MakeImage(Rect rect, std::string path) {
    return Image(rect, path);
}

bool Image::operator==(const Image& other) const {
// 仅比较图片的长宽和路径（位置不重要），如果相等，他们可以分享同一个VulkanImageInfo
//return rect_.w_ == other.rect_.w_ && rect_.h_ == other.rect_.h_ &&
//        path_ == other.path_;

    // 仅仅比较路径
    return path_ == other.path_;
}