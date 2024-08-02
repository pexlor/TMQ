#include <cassert>
#include "TMQueue.h"
#include "Defines.h"

USING_TMQ_NAMESPACE

void testPush()
{
    LOG_DEBUG("test queue, method push");
    TMQueue queue;
    TMsg msg;
    queue.Push(msg);
    assert(true);
}

void testPick()
{
    LOG_DEBUG("test queue, method pick");
    TMQueue queue;
    TMsg msg;
    queue.Push(msg);
    TMsg pick;
    assert(queue.Pick(nullptr, 0, pick) == false);

    const char *topic = "test";
    const char *data = "hello from test";
    TMsg msgTest(topic, TMQMsg(data, (long)strlen(data) + 1));
    queue.Push(msgTest);
    const char *topics[1];
    topics[0] = topic;
    assert(queue.Pick(topics, 1, pick));
    assert(strcmp(pick.topic, topic) == 0);
    assert(strcmp((char *)pick.data, data) == 0);
    assert(pick.length == (long)strlen(data) + 1);

    int it = 1000;
    TMQueue itQueue;
    for (int i = 0; i < it; ++i)
    {
        itQueue.Push(msgTest);
    }
    assert(itQueue.Size() == it);
    for (int i = 0; i < it; ++i)
    {
        assert(itQueue.Pick(topics, 1, pick));
        assert(strcmp(pick.topic, topic) == 0);
        assert(strcmp((char *)pick.data, data) == 0);
        assert(pick.length == (long)strlen(data) + 1);
    }
    assert(itQueue.Size() == 0);

    LOG_DEBUG("test queue, method pick, pass");
}

void testPoll()
{
    LOG_DEBUG("test queue, method poll");
    // push and poll
    TMQueue queue;
    const char *topic = "test";
    const char *data = "hello from test";
    TMsg msgTest(topic, TMQMsg(data, (long)strlen(data) + 1));
    queue.Push(msgTest);
    TMsg polled;
    queue.Poll(polled, nullptr, 0);
    assert(strcmp(polled.topic, msgTest.topic) == 0);
    assert(polled.id == msgTest.id);
    assert(polled.length == msgTest.length);
    assert(memcmp(polled.data, msgTest.data, polled.length) == 0);
    assert(queue.Size() == 0);

    // push and poll with exclude topics
    const char *topics[1];
    topics[0] = topic;
    TMsg exPoll;
    queue.Push(msgTest);
    assert(queue.Poll(exPoll, topics, 1) == false);
    assert(exPoll.length == 0);
    assert(queue.Poll(exPoll, nullptr, 0) == true);
    assert(exPoll.length == msgTest.length);
    LOG_DEBUG("test queue, method poll, pass");
}

void testPriority()
{
    LOG_DEBUG("test queue, function priority");
    TMQueue queue;
    const char *topic = "test";
    const char *data = "hello from test";
    TMsg msgTest(topic, TMQMsg(data, (long)strlen(data) + 1));
    msgTest.priority = 5;
    queue.Push(msgTest);
    strcpy(msgTest.topic, "priority");
    msgTest.priority = 6;
    queue.Push(msgTest);

    TMsg pick;
    const char *topics[2];
    topics[0] = topic;
    topics[1] = "priority";
    assert(queue.Pick(topics, 2, pick) == true);
    assert(strcmp(pick.topic, "priority") == 0);
    assert(queue.Pick(topics, 2, pick) == true);
    assert(strcmp(pick.topic, "test") == 0);
    assert(queue.Size() == 0);
    LOG_DEBUG("test queue, function priority, pass");
}

void testQueue()
{
    LOG_DEBUG("test queue, start.");
    testPush();
    testPick();
    testPoll();
    testPriority();
    LOG_DEBUG("test queue, finish.");
}