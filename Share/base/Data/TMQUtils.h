//
//  TMQUtils.h
//  TMQUtils
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_UTILS_H
#define TMQ_UTILS_H

#include "Defines.h"

TMQ_NAMESPACE

// TMQ Util functions

    class TMQUtils {
    public:
        /**
         * Trim blank space at both of the beginning and end.
         * @param str, a pointer to a string to trim.
         * @param len, the length of the original string.
         * @param data, a pointer to the string pointer, which will be used to store the result.
         * @return, the length of the string pointed by the result *data.
         *  Attentions, this method will not create a new string, but point to the original string. So
         *  the *data will point to any position on original string and the return length will be small
         *  than the total length.
         */
        static int Trim(const char *str, int len, const char **data);

        /**
         * Convert string to integer.
         * @param str, a pointer to the string.
         * @param len, the length of the string.
         * @param res, a pointer to the result.
         * @return, a boolean value indicate whether the conversion is success or not. If the conversion
         * is failed, the result value pointed by res is undefined.
         */
        static bool ToInt(const char *str, int len, int *res);
    };

TMQ_NAMESPACE_END

#endif //TMQ_UTILS_H
