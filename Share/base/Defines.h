#ifndef APP_DEFINES_H
#define APP_DEFINES_H

// 定义一个宏TAG，用于日志输出的标识
#define TAG "BaseService"

// 根据编译平台的不同，定义不同的日志输出宏
#ifdef __ANDROID__
    // 如果是Android平台，使用android/log.h中的__android_log_print函数进行日志输出
    #include "android/log.h"
    #define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#elif __APPLE__
    // 如果是Apple平台（如iOS或macOS），则不进行日志输出（或者可以在这里添加自定义的日志输出实现）
    #define LOG_DEBUG(...) 
#endif

// 定义TMQ命名空间开始和结束的宏
#define TMQ_NAMESPACE namespace TMQ {
#define TMQ_NAMESPACE_END }

// 定义一个宏，用于简化TMQ命名空间的使用
#define USING_TMQ_NAMESPACE using namespace TMQ;

#endif // APP_DEFINES_H