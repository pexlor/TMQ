//
//  TestSuite.cpp
//  TestSuite
//
//  Created by  on 2022/8/21.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include "Defines.h"
#include <cstdio>
#include <cstdlib>

#define LOG_TEST_INFO LOG_DEBUG
#define LOG_TEST_START(s)   LOG_DEBUG("==================================%s",s)
#define LOG_TEST_FINISH(s)  LOG_DEBUG("==================================%s OK", s)

#if defined(__ANDROID__)
#define STORAGE_TEST_FILE   "/data/data/com.link.invoker/storage_test"
#define PERSIST_TEST_FILE   "/data/data/com.link.invoker/persist_test"
#elif defined(__APPLE__)
#define STORAGE_TEST_FILE   ""
#define PERSIST_TEST_FILE   ""
#else
#define STORAGE_TEST_FILE   ""
#define PERSIST_TEST_FILE   ""
#endif

#define ASSERT_TRUE(x, msg)                 \
        do{                                 \
            if(!(x))                        \
            {                               \
                LOG_TEST_INFO(msg);         \
                abort();                    \
            }                               \
        }while(0)

// print test log
class TestLog {
    const char *fun;
public:
    TestLog(const char *fun) {
        this->fun = fun;
        LOG_TEST_START(fun);
    }

    ~TestLog() {
        LOG_TEST_FINISH(fun);
    }
};

#define LOG_TEST_ENTRY() TestLog testLog(__FUNCTION__)
#endif //TEST_SUITE_H
