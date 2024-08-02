#ifndef ANDROID_TMQUTILS_H
#define ANDROID_TMQUTILS_H

#include "Defines.h"

TMQ_NAMESPACE

class TMQUtils {
public:
    static int Trim(const char* str, int len, const char** data);
    static bool ToInt(const char* str, int len, int*res);
};

TMQ_NAMESPACE_END

#endif //ANDROID_TMQUTILS_H