/
// Created by junhui on 2022/7/13.
//

#include <cassert>
#include <cstring>

#include "Defines.h"

#include "TMQTopic.h"
#include "TMQFactory.h"

void testGetInstance()
{
    LOG_DEBUG("test Topic, method GetInstance");
    assert(TMQFactory::GetTopicInstance() != nullptr);
    TMQTopic *topic = TMQFactory::GetTopicInstance();
    assert(topic == TMQFactory::GetTopicInstance());
    LOG_DEBUG("test Topic, method GetInstance, pass");
}

void testPublish()
{
    LOG_DEBUG("test Topic, method publish");
    const char *topic = "TestTopic";
    const char *data = "hello tmq";
    unsigned int len = strlen(data) + 1;
    TMQMsg tmqMsg(data, (long)len);
    TMQFactory::GetTopicInstance()->Publish(topic, tmqMsg);
    TMQFactory::GetTopicInstance()->Publish(topic, (void *)data, (long)len);
    TMQFactory::GetTopicInstance()->Publish(topic, (void *)data, (long)len, 1, 5);
    LOG_DEBUG("test Topic, method publish, pass");
}

class TestReceiver : public TMQReceiver
{

    void OnReceive(const TMQMsg *msg) override
    {
        LOG_DEBUG("receive dispatcher message: %s", (char *)msg->data);
    }
};

void testSubscribe()
{
    LOG_DEBUG("test Topic, method subscribe");
    long sid = TMQFactory::GetTopicInstance()->Subscribe("TestTopic", new TestReceiver());
    assert(sid > 0);
    LOG_DEBUG("test Topic, method subscribe, pass");
}

void testUnsubscribe()
{
    LOG_DEBUG("test Topic, method unsubscribe");
    long sid = TMQFactory::GetTopicInstance()->Subscribe("UnTestTopic", new TestReceiver());
    assert(sid > 0);
    assert(TMQFactory::GetTopicInstance()->UnSubscribe(sid) == true);
    LOG_DEBUG("test Topic, method unsubscribe, pass");
}

void testTopic()
{
    LOG_DEBUG("test Topic, start");
    testGetInstance();
    testSubscribe();
    testPublish();
    testUnsubscribe();
    LOG_DEBUG("test Topic, finish");
}