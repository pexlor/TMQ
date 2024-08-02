#include <cassert>
#include <cstring>
#include <cstdio>

#include "Defines.h"
#include "TMQC.h"

void test_malloc_and_free()
{
    LOG_DEBUG("test TMQC, method malloc and free.");
    void *buf = tmq_malloc(10);
    memset(buf, 0, 10);
    memcpy(buf, "test", 4);
    assert(strcmp((char *)buf, "test") == 0);
    tmq_free(buf);
    LOG_DEBUG("test TMQC, method malloc and free, pass");
}

void test_context()
{
    LOG_DEBUG("test TMQC, method context");
    long ctx = tmq_create_ctx(nullptr);
    tmq_destroy_ctx(ctx);
    LOG_DEBUG("test TMQC, method context, pass");
}

void TMQCallback(void *data, long length, long customId)
{
    LOG_DEBUG("receive message from c callback: %s", (char *)data);
}

void test_context_pub()
{
    LOG_DEBUG("test TMQC, method publish & pick");
    long ctx = tmq_create_ctx(TMQCallback);
    const char *topic = "TestCPublish";
    tmq_ctx_subscribe(ctx, topic);

    const char *data = "hello from tmq c";
    long len = (long)strlen(data) + 1;
    long mid = tmq_ctx_publish(ctx, (void *)data, len);
    assert(mid > 0);
    mid = tmq_ctx_publish(ctx, (void *)data, len, 1, 5);
    assert(mid > 0);
    char *res;
    long length = tmq_ctx_pick(ctx, (void **)&res);
    assert(len == length);
    assert(strcmp(data, res) == 0);
    tmq_free(res);

    int it = 200;
    char buf[128] = {0};
    for (int i = 0; i < it; ++i)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "hello from tmq c test, %d", i);
        assert(tmq_ctx_publish(ctx, (void *)buf, sizeof(buf)) > 0);
    }

    tmq_destroy_ctx(ctx);
    LOG_DEBUG("test TMQC, method publish & pick, pass");
}

void test_context_pick()
{
    LOG_DEBUG("test TMQC, method pick");
    long ctx = tmq_create_ctx(nullptr);
    const char *topic = "TestCPick";
    tmq_ctx_subscribe(ctx, topic);

    const char *data1 = "hello from tmq c 1";
    long len1 = (long)strlen(data1) + 1;
    long mid1 = tmq_ctx_publish(ctx, (void *)data1, len1, 1, 5);
    assert(mid1 > 0);
    const char *data2 = "hello from tmq c 2";
    long len2 = (long)strlen(data1) + 1;
    long mid2 = tmq_ctx_publish(ctx, (void *)data2, len2, 1, 6);
    assert(mid2 > 0);

    // data2 has high priority
    char *res;
    long length = tmq_ctx_pick(ctx, (void **)&res);
    assert(len2 == length);
    assert(strcmp(data2, res) == 0);
    tmq_free(res);

    // data1 will be pick
    length = tmq_ctx_pick(ctx, (void **)&res);
    assert(len1 == length);
    assert(strcmp(data1, res) == 0);
    tmq_free(res);

    tmq_destroy_ctx(ctx);
    LOG_DEBUG("test TMQC, method pick, pass");
}

void testC()
{
    LOG_DEBUG("test c, start");
    test_malloc_and_free();
    test_context();
    test_context_pub();
    test_context_pick();
    LOG_DEBUG("test c, finish");
}