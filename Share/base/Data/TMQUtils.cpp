//
//  TMQUtils.cpp
//  TMQUtils
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQUtils.h"

USING_TMQ_NAMESPACE

// Calculate the real content for string without the blank space at the beginning and the end.
int TMQUtils::Trim(const char *str, int len, const char **data) {
    // Check parameters valid.
    if (str == nullptr || len <= 0) {
        return -1;
    }
    // Find the real content at the beginning.
    int start = 0;
    int end = len - 1;
    for (int i = 0; i < len; ++i) {
        // Check whether the str[i] is blank space. If it is false, the loop will break.
        if (str[i] == ' ') {
            start++;
        } else {
            break;
        }
    }
    // Find the real content at the end from end to start.
    for (int i = len - 1; i > start; --i) {
        // Check whether the str[i] is blank space. If it is false, the loop will break.
        if (str[i] == ' ') {
            end--;
        } else {
            break;
        }
    }
    // Assign the data pointer to str + start and return the length of the real content.
    *data = str + start;
    return end - start + 1;
}

/*
 * Convert string to int.
 */
bool TMQUtils::ToInt(const char *str, int len, int *res) {
    // check if valid
    if (str == nullptr || len <= 0) {
        return false;
    }
    // Calculate the sign at first.
    int sign = 1;
    int start = 0;
    if (str[0] == '-') {
        sign = -1;
        start = 1;
    }
    // For 0-9, convert them into integer.
    int result = 0;
    for (int i = start; i < len; ++i) {
        if (str[i] - '0' < 0 || str[i] - '9' >= 0) {
            return false;
        }
        result = result * 10 + (str[i] - '0');
    }
    *res = sign * result;
    return true;
}