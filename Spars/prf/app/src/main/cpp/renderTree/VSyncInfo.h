//
// Created by richardwu on 11/7/24.
//

#ifndef PRF_VSYNCINFO_H
#define PRF_VSYNCINFO_H

#include <cstdint>

// TODO: 当前未接入vsync NDK，暂时仍用VSyncInfo提供的时间戳
struct VSyncInfo {
    uint64_t frameIndex;
    int64_t vsyncTS;
};

#endif //PRF_VSYNCINFO_H
