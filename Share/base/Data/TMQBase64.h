//
//  TMQBase64.h
//  TMQBase64
//
//  Created by  on 2022/9/22.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_BASE64_H
#define TMQ_BASE64_H

#include "Defines.h"

TMQ_NAMESPACE

    class TMQBase64 {
    public:
        static int EncodeLength(int length);

        static int Encode(char *codedDst, const char *plainSrc, int lenPlainSrc);

        static int DecodeLength(const char *codedSrc);

        static int Decode(char *plainDst, const char *codedSrc);
    };

TMQ_NAMESPACE_END

#endif //TMQ_BASE64_H
