//
//  TMQTopic.h
//  TMQTopic
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_TOPIC_H
#define TMQ_TOPIC_H

// constant value of the tmq version
#define TMQ_VERSION                 "1.1.0"

// length limit of the tmq topic string
#define TMQ_TOPIC_MAX_LENGTH        128

// tmq msg type for flag
#define TMQ_MSG_TYPE_PICK           0x00000001
#define TMQ_MSG_TYPE_DISPATCH       0x00000002
#define TMQ_MSG_TYPE_ALL            0x0000ffff
// Default message priority
#define PRIORITY_NORMAL             5

// Id for unsigned long long integer.
typedef unsigned long long TMQMsgId;
// Common id for long integer.
typedef long TMQId;
// Redefine unsigned int as TMQSize
typedef unsigned int TMQSize;

/**
 * class for TMQMsg, wrapper for the binary data and necessary properties.
 */
class TMQMsg {
public:
    /**
     * tmq msg id, this id will be generated after the message published to tmq.
     * this message id has a long type, which can differ on different machine.
     */
    TMQMsgId msgId;
    // the length of the data, should be greater than 0.
    int length;
    // the pointer for the binary data
    void *data;
    // priority of the message, will be range at [0, 10]
    int priority;
    // flag of the tmq message, refer TMQ_MSG_TYPE_{XXX} for detail.
    int flag;
public:
    // default construct for TMQMsg.
    TMQMsg();

    // Construct a tmq msg object with data pointer and its length.
    TMQMsg(const void *data, int length);

    /**
     * Construct with a tmqMsg.
     * @param tmqMsg
     */
    TMQMsg(const TMQMsg &tmqMsg);

    /**
     * Override of the assign operator
     * @param msg original tmq msg
     * @return a new msg with deep copy.
     */
    TMQMsg &operator=(const TMQMsg &msg);

    /**
     * virtual destruct method.
     */
    virtual ~TMQMsg();
};

/**
 * Interface defined for message picker. A picker can be used to pick message from tmq instance.
 * One picker can include one or more topics, any message with the same topic will be picked by
 * this picker.
 *
 * The advantage of using picker for consume messages is that the picker can record the position of
 * the last pick and search again from that position. Pick message by picker can have more efficient
 */
class IPicker {
public:
    /**
     * Pick a topic message
     * @param topic, the topic of the picked message.
     * @param tmqMsg, the content wrapper class for the picked message.
     * @return bool, a boolean value indicate the pick is success or not.
     */
    virtual bool Pick(char *topic, TMQMsg &tmqMsg) = 0;

    /**
     * virtual method of destructor.
     */
    virtual ~IPicker() {}
};

/**
 * Topic message receiver, an interface defined for receive a topic message. It is used during
 * message dispatch.
 * You can implement this interface and then register it to the tmq. When there is a topic message
 * arrived, dispatcher will invoke the OnReceive method.
 */
class TMQReceiver {
public:
    /**
     * The virtual method for receive a message, user should implement it.
     * @param msg, the message dispatched by tmq.
     * @return, void.
     */
    virtual void OnReceive(const TMQMsg *msg) = 0;

    /**
     * virtual method of destructor.
     */
    virtual ~TMQReceiver() {}
};

/**
 * API methods for tmq. It is a virtual class, defined as interface. You can get an instance by
 * TMQFactory. These methods can be divide into four categories: subscribe, picker, publish
 * and history messages.
 *
 * All these methods are thread safe, invoke them as needed.
 *
 */
class TMQTopic {
public:
    /**
     * Subscribe topic messages. A receiver binds to one topic, any of this topic message arrived,
     * the receiver will be called. It is recommend to deal with the message as soon as possible.
     * @param topic, the topic to be subscribed
     * @param receiver, the receiver for dealing with the messages.
     * @return long, a long value represent this subscriber id.
     */
    virtual TMQId Subscribe(const char *topic, TMQReceiver *receiver) = 0;

    /**
     * Cancel a subscription. This method may be time consuming, when the subscriber is on running.
     * @param subscribeId, The subscriber id returned by Subscribe
     * @return bool, a boolean value indicates whether the invoke is success or not.
     */
    virtual bool UnSubscribe(TMQId subscribeId) = 0;

    /**
     * Find a receiver by subscriber id.
     * @param subscribeId, the id of a subscriber.
     * @return a TMQReceiver pointer, if not exist, the return will be nullptr.
     */
    virtual TMQReceiver *FindSubscriber(TMQId subscribeId) = 0;

    /**
     * Create a tmq picker for the message queue of tmq. After use, invoke destroy method timely.
     * @param topics, an pointer for the topic array, one or more topics.
     * @param len, the length of the topics.
     * @param type, the picker type, will apply to tmq message flag with &(logical and operation)
     *  if the result of the and operator is not zero, it will compare the topics continuously.
     * @return IPicker*, a pointer to the picker.
     */
    virtual IPicker *CreatePicker(const char **topics, int len, int type) = 0;

    /**
     * Destroy the topic message picker.
     * @param picker, a pointer to the picker created by CreatePicker.
     * @return void, nothing.
     */
    virtual void DestroyPicker(IPicker *picker) = 0;

    /**
     * Publish a binary data with topic, binary data and flag.
     * @param topic, the topic for this message.
     * @param data, the pointer to the binary data.
     * @param length, the length of the data.
     * @param flag, the message flag, refer TMQ_MSG_TYPE_XXX for detail.
     * @param priority, message priority, all priority values can be [0, 10]
     * @return TMQMsgId, a TMQMsgId type value indicate the id of the msg.
     */
    virtual TMQMsgId Publish(const char *topic, void *data, int length, int flag = 0,
                         int priority = PRIORITY_NORMAL) = 0;

    /**
     * Publish a tmq message. The content of this message pointed by msg.
     * @param topic, the topic for this message.
     * @param msg, the tmq msg
     * @return TMQMsgId, a TMQMsgId type value indicate the id of the msg.
     */
    virtual TMQMsgId Publish(const char *topic, const TMQMsg &msg) = 0;

    /**
     * Enable persistent storage for tmq messages. If the enable is true, the parameter file should
     * be valid file path, the file associated with the path must have read/write permissions. If
     * the file is not exist, it will be created. Besides, if the enable is false, and the
     * persistent instance is running, it will clear the persistence and clear its file.
     * @param enable, a boolean value indicate whether the persistent is enable or not.
     * @param file, a pointer to file path, max length limits to 255.
     * @return bool, a boolean value for whether the persistent is enable or not.
     */
    virtual bool EnablePersistent(bool enable, const char *file) = 0;

    /**
     * Get the history message for some topics. The history messages is the kind of message
     *  that picked or dispatched. The history messages are all resource constrained,
     *  so every kind of history message are not unlimited, refer to TMQHistory for more detail.
     * @param topics, the topic array includes one or more topics.
     * @param len, the length of the topics.
     * @param values, the pointer to the message array, which is assigned by this method
     *  when there are history messages. the length for this message array will be as a return value.
     * @return long, a long type value indicate the length of *values.
     */
    virtual TMQSize GetHistory(const char **topics, int len, TMQMsg **values) = 0;
};


#endif //TMQ_TOPIC_H
