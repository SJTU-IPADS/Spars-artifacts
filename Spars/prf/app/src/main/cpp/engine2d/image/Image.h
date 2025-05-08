//
// Created by richardwu on 11/15/24.
//

#ifndef PRF_IMAGE_H
#define PRF_IMAGE_H

#include <string>
#include "../rects/Rect.h"
#include "../ImageManager.h"

class Image {
public:
    Rect rect_;
    std::string path_;

    static Image MakeImage(Rect rect, std::string path);

    /* ImageManager中会将Image作为key，查找VulkanImageInfo */
    bool operator==(const Image& other) const;

private:
    Image(Rect rect, std::string path);

};

#endif //PRF_IMAGE_H
