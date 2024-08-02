//
//  TMQBase64.cpp
//  TMQBase64
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

/*
 * Base64 encoder/decoder. Originally Apache file ap_base64.c
 */

#include <cstring>
#include "TMQBase64.h"

USING_TMQ_NAMESPACE

/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] =
        {
                /* ASCII table */
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
                64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
                64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
        };

int TMQBase64::DecodeLength(const char *codedSrc) {
    int nBytesDecoded;
    const unsigned char *bufIn;
    int nprBytes;

    bufIn = (const unsigned char *) codedSrc;
    while (pr2six[*(bufIn++)] <= 63);

    nprBytes = (bufIn - (const unsigned char *) codedSrc) - 1;
    nBytesDecoded = ((nprBytes + 3) / 4) * 3;

    return nBytesDecoded + 1;
}

int TMQBase64::Decode(char *plainDst, const char *codedSrc) {
    int nBytesDecoded;
    const unsigned char *bufIn;
    unsigned char *bufOut;
    int nprBytes;

    bufIn = (const unsigned char *) codedSrc;
    while (pr2six[*(bufIn++)] <= 63);
    nprBytes = (bufIn - (const unsigned char *) codedSrc) - 1;
    nBytesDecoded = ((nprBytes + 3) / 4) * 3;

    bufOut = (unsigned char *) plainDst;
    bufIn = (const unsigned char *) codedSrc;

    while (nprBytes > 4) {
        *(bufOut++) =
                (unsigned char) (pr2six[*bufIn] << 2 | pr2six[bufIn[1]] >> 4);
        *(bufOut++) =
                (unsigned char) (pr2six[bufIn[1]] << 4 | pr2six[bufIn[2]] >> 2);
        *(bufOut++) =
                (unsigned char) (pr2six[bufIn[2]] << 6 | pr2six[bufIn[3]]);
        bufIn += 4;
        nprBytes -= 4;
    }

    /* Note: (nprBytes == 1) would be an error, so just ingore that case */
    if (nprBytes > 1) {
        *(bufOut++) =
                (unsigned char) (pr2six[*bufIn] << 2 | pr2six[bufIn[1]] >> 4);
    }
    if (nprBytes > 2) {
        *(bufOut++) =
                (unsigned char) (pr2six[bufIn[1]] << 4 | pr2six[bufIn[2]] >> 2);
    }
    if (nprBytes > 3) {
        *(bufOut++) =
                (unsigned char) (pr2six[bufIn[2]] << 6 | pr2six[bufIn[3]]);
    }

    *(bufOut++) = '\0';
    nBytesDecoded -= (4 - nprBytes) & 3;
    return nBytesDecoded;
}

static const char basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int TMQBase64::EncodeLength(int len) {
    return ((len + 2) / 3 * 4) + 1;
}

int TMQBase64::Encode(char *codedDst, const char *plainSrc, int lenPlainSrc) {
    if (!codedDst) {
        return -1;
    }
    int i;
    char *p;
    p = codedDst;
    for (i = 0; i < lenPlainSrc - 2; i += 3) {
        *p++ = basis64[(plainSrc[i] >> 2) & 0x3F];
        *p++ = basis64[((plainSrc[i] & 0x3) << 4) |
                       ((int) (plainSrc[i + 1] & 0xF0) >> 4)];
        *p++ = basis64[((plainSrc[i + 1] & 0xF) << 2) |
                       ((int) (plainSrc[i + 2] & 0xC0) >> 6)];
        *p++ = basis64[plainSrc[i + 2] & 0x3F];
    }
    if (i < lenPlainSrc) {
        *p++ = basis64[(plainSrc[i] >> 2) & 0x3F];
        if (i == (lenPlainSrc - 1)) {
            *p++ = basis64[((plainSrc[i] & 0x3) << 4)];
            *p++ = '=';
        } else {
            *p++ = basis64[((plainSrc[i] & 0x3) << 4) |
                           ((int) (plainSrc[i + 1] & 0xF0) >> 4)];
            *p++ = basis64[((plainSrc[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';
    return p - codedDst;
}