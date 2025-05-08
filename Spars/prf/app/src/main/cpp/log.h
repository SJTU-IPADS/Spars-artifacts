//
// Created by richardwu on 2024/10/18.
//

#ifndef PRF_LOG_H
#define PRF_LOG_H

#include <android/log.h>

// Android log function wrappers
#define KTAG "prf-android"
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, KTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, KTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, KTAG, __VA_ARGS__))

#endif //PRF_LOG_H
