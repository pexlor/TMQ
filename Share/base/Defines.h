#ifndef APP_DEFINES_H
#define APP_DEFINES_H
// TMQ namespace
#define TMQ_NAMESPACE namespace TMQ{
#define TMQ_NAMESPACE_END }
#define USING_TMQ_NAMESPACE using namespace TMQ;
// TMQ namespace end.
/// log define for different platform.
#define TAG "TMQ"
#ifdef __ANDROID__

#include "android/log.h"

#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_INFO, TAG ,__VA_ARGS__)
#elif __APPLE__
#define LOG_DEBUG(...) 
#endif

// Redefine strncpy to strncpy_s on windows, to solve the problem:
// C4996	'strncpy': This function or variable may be unsafe. Consider using strncpy_s instead.
#if _WINDOWS
#define strncpy strncpy_s
#endif

/// TMQ inner ids
// Id for unsigned long long integer, using for tmq message id.
typedef unsigned long long TMQMsgId;
// Redefine unsigned int as TMQSize
typedef unsigned int TMQSize;
// Redefine unsigned int as TMQSize
typedef unsigned long TMQLongSize;
// Id for unsigned long long integer.
typedef unsigned long long TMQMsgId;
// Common id for long integer.
typedef long TMQId;
// This is the invalid value define.
#define ID_INT_INVALID              -1
#define ID_LONG_INVALID             -1
#define MSG_LENGTH_INVALID          -1
#define SIZE_INVALID                -1

/// The priority defined for tmq message.
// Total length of the tmq msg priority is TMQ_PRIORITY_COUNT. The higher value of the priority will
// have higher priority, and lower value will have lower priority.
#define TMQ_PRIORITY_COUNT      10
// Default priority for a message. It is set to the middle value of TMQ_PRIORITY_COUNT, which has
// the normal priority.
#define TMQ_PRIORITY_DEFAULT    5

/// logic defined operation
// IS_PERSIST indicates whether the tmq message is persistent of not.
#define IS_PERSIST(flag)        (flag & 0x40000000)
// FORCE_PERSIST, this operation can set the persistent flag.
#define FORCE_PERSIST(flag)     (flag | 0x40000000)
// IS_MEMORY indicate whether the tmq message is saved into memory or not.
#define IS_MEMORY(flag)         (flag & 0x80000000)
// FORCE_MEMORY, set up the memory flag
#define FORCE_MEMORY(flag)      (flag | 0x80000000)
// GET_MSG_TYPE, get the message type from the flag. The low 16 bits represent the type of a message.
#define GET_MSG_TYPE(flag)      (flag & 0x0000ffff)
#define FORCE_TYPE_ALL(flag)    (flag | 0x0000ffff)

#endif //APP_DEFINES_H
