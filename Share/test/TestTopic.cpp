//
//  TestTopic.cpp
//  TestTopic
//
//  Created by  on 2022/8/21.
//  Copyright (c)  Tencent. All rights reserved.
//

#include <unistd.h>
#include "TestSuite.h"
#include "Topic.h"

USING_TMQ_NAMESPACE

class TopicTestReceiver : public TMQReceiver {
public:
    volatile int count = 0;
public:
    void OnReceive(const TMQMsg *msg) override {
        count++;
    }
};

void TestTopicStorage() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    ASSERT_TRUE(topicInst.GetStorage() != nullptr,
                "Storage instance in topicInst should not be null");
}

void TestPublishData() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    const char *topic = "TestTopic";
    const char *data = "This is the test data";
    TMQMsgId msgId = topicInst.Publish(topic, (void *) data, strlen(data) + 1, TMQ_MSG_TYPE_PICK);
    ASSERT_TRUE(msgId > 0, "The return value by publish data should be valid.");
}

void TestPublishTmqMsg() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    const char *topic = "TestTopic";
    const char *data = "This is the test data";
    TMQMsg tmqMsg(data, strlen(data) + 1);
    tmqMsg.flag = TMQ_MSG_TYPE_PICK;
    TMQMsgId msgId = topicInst.Publish(topic, tmqMsg);
    ASSERT_TRUE(msgId > 0, "The return value by publish tmq msg should be valid.");
}

void TestFindQueue() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    for (int i = 0; i < TMQ_PRIORITY_COUNT; ++i) {
        ASSERT_TRUE(topicInst.FindQueue(i)->Size() == 0,
                    "When there is no message, size of the queue should be zero.");
    }
}

void TestCreatePicker() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    const char *topic = "TestTopic";
    IPicker *picker = topicInst.CreatePicker(&topic, 1, TMQ_MSG_TYPE_ALL);
    ASSERT_TRUE(picker != nullptr, "Create a picker should be success.");
    topicInst.DestroyPicker(picker);
}

void TestPickEmpty() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    const char *topic = "TestTopic";
    IPicker *picker = topicInst.CreatePicker(&topic, 1, TMQ_MSG_TYPE_ALL);
    char pickedTopic[TMQ_TOPIC_MAX_LENGTH] = {0};
    TMQMsg tmqMsg;
    bool suc = picker->Pick(pickedTopic, tmqMsg);
    ASSERT_TRUE(!suc, "We have not publish tmq msg, so pick should return fail.");
    topicInst.DestroyPicker(picker);
}

void TestPickMsg() {
    LOG_TEST_ENTRY();
    Topic topicInst;
    const char *topic = "TestTopic";
    const char *data = "This is the test data";
    TMQMsgId msgId = topicInst.Publish(topic, (void *) data, strlen(data) + 1, TMQ_MSG_TYPE_PICK);
    IPicker *picker = topicInst.CreatePicker(&topic, 1, TMQ_MSG_TYPE_ALL);
    char pickedTopic[TMQ_TOPIC_MAX_LENGTH] = {0};
    TMQMsg tmqMsg;
    bool suc = picker->Pick(pickedTopic, tmqMsg);
    topicInst.DestroyPicker(picker);
    ASSERT_TRUE(suc,
                "We have publish a tmq msg, so pick should return success.");
    ASSERT_TRUE(tmqMsg.msgId == msgId,
                "Picked tmq msg id should be equal to msg id by publish.");
}

void TestSubscribe() {
    LOG_TEST_ENTRY();
    const char *topic = "TestTopic";
    Topic topicInst;
    TMQId rid = topicInst.Subscribe(topic, new TopicTestReceiver());
    bool suc = topicInst.UnSubscribe(rid);
    ASSERT_TRUE(rid > 0, "The id return by Subscribe should be valid.");
    ASSERT_TRUE(suc, "UnSubscribe a valid receiver id, the result should be success.");
}

void TestSubscribeAndReceive() {
    LOG_TEST_ENTRY();
    const char *topic = "TestTopic";
    const char *data = "This is data.";
    Topic topicInst;
    TopicTestReceiver receiver;
    TMQId rid = topicInst.Subscribe(topic, &receiver);
    topicInst.Publish(topic, (void *) data, strlen(data) + 1, TMQ_MSG_TYPE_DISPATCH);
    LOG_TEST_INFO("Waiting msg dispatched by tmq...");
    while (receiver.count == 0);
    bool suc = topicInst.UnSubscribe(rid);
    ASSERT_TRUE(receiver.count == 1, "The count of received msg should be equal to published.");
    ASSERT_TRUE(suc, "UnSubscribe a valid receiver id, the result should be success.");
}

void TestTopic() {
    TestTopicStorage();
    TestPublishData();
    TestPublishTmqMsg();
    TestCreatePicker();
    TestFindQueue();
    TestPickEmpty();
    TestPickMsg();
    TestSubscribe();
    TestSubscribeAndReceive();
}
