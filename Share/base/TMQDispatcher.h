//
//  TMQDispatcher.h
//  TMQDispatcher
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_DISPATCHER_H
#define TMQ_DISPATCHER_H

#include "Defines.h"
#include "Topic.h"
#include "Executor.h"
#include "TMQMutex.h"
#include "Shadow.h"

/// Const definitions
// Default executor count for the dispatcher. This is not optimal. The count of the executor should
// be related to the CPUs. But what we should know also is that the dispatcher can be adaptive
// itself with the message congestion.
#define DEFAULT_EXECUTOR_COUNT              4
// max count of the remained messages for starting a new executor.
#define EXECUTOR_WAKE_COUNT                 512
// const string for sent topic
#define TOPIC_SENT                          "__SENT__"

TMQ_NAMESPACE

/*
 * Dispatcher receiver, used to wrapper the TMQReceiver and its unique id.
 */
    class TMQDispatcherReceiver {
    public:
        // a tmq receiver
        TMQReceiver *receiver;
        // the id for this receiver
        TMQId id;
    public:
        TMQDispatcherReceiver(TMQReceiver *tmqReceiver, TMQId id) {
            this->receiver = tmqReceiver;
            this->id = id;
        }
    };

/**
 * A topic receiver list for saving receivers with its topic.
 */
    class TMQTopicReceivers {
    private:
        // topic for the receivers.
        char name[TMQ_TOPIC_MAX_LENGTH] = {0};
        // receiver list.
        List<TMQDispatcherReceiver *> receivers;
    public:
        /**
         * Constructor a TMQTopicReceivers with name.
         * @param name
         */
        TMQTopicReceivers(const char *name = nullptr) {
            if (name) {
                strncpy(this->name, name, sizeof(this->name));
            }
        }

        /*
         * Get method for the name.
         */
        const char *GetName() {
            return name;
        }

        /**
         * Set method for name.
         * @param s, a pointer to the topic
         */
        void SetName(const char *topic) {
            if (topic) {
                strncpy(this->name, topic, sizeof(this->name));
            }
        }

        /*
         * Get method for the topic receiver list.
         */
        List<TMQDispatcherReceiver *> *GetReceivers() {
            return &receivers;
        }
    };

/**
 *
 * TMQ dispatcher implementation for the interface of Dispatcher and TMQCallable.
 * This is class that uses to manage the topic receivers and thread executors.
 *
 *  For receivers:
 * It has an topic receiver list allowed to add, find and remove a topic receiver. All receivers are
 * organized by their subscribed topic, which can find a topic receiver more quick. The receivers
 * under one topic are ordered with fifo. Receivers registered more early will be invoked more fast.
 *
 * For executors:
 * A dispatcher contains zero or several executors. if There are no any executors, maybe it is
 * limited by user for resource concern, a message can wakeup itself thread for dispatching. This is
 * very useful for platform that can not be running on the background, such as iOS. During the last
 * publish of the tmq message before app entering into background, it can wakeup itself for the last
 * message dispatching. Another thing, it is flexible for the executors. when the messages is not
 * congested, there may be only one executor running. And when there are many messages congested,
 * the dispatcher will startup new executor for sending message.
 *
 */
    class TMQDispatcher : public Dispatcher, TMQCallable {
    private:
        // TMQ topic instance of the message pool.
        TMQTopic *tmqTopic;
        // dispatcher mutex for topic executors.
        TMQMutex mutex;
        // The max count limits for executors.
        int maxExecutorCount;
        // The active count of the executors.
        int activeExecutorCount;
        // The pointer to the executor array.
        IExecutor **executors;
        // Dispatch mutex for running receivers.
        TMQMutex dispatcherMutex;
        // receiverMutex, for topic receivers only
        TMQMutex receiverMutex;
        // Topic receiver list, organized by subscribed topic. That means one topic has multiple receivers.
        List<TMQTopicReceivers *> topicReceivers;
        // Running receivers, for waiting when unsubscribe
        List<TMQDispatcherReceiver *> runningReceivers;

        // Counter for wakeup
        TMQLongSize wakeupCount;
        // Counter for message sent.
        TMQLongSize sentCount;
        // bool value, indicates whether this invoke can wakeup itself.
        volatile bool forceActiveSelf;
        // Generate a id for the registered receiver.
        TMQId receiverIdCounter;
        // The picker using for pick message from tmq.
        IPicker *picker;
        // Indicates whether to stop the running or not. It is a volatile variable.
        volatile bool stop;
    public:
        /**
         * Constructor for tmq dispatcher.
         * @param topic the pointer to the tmq topic instance.
         * @param maxExecutorCount, the max limit for the count of the executors.
         *  Default value is DEFAULT_EXECUTOR_COUNT
         */
        TMQDispatcher(TMQTopic *topic, int maxExecutorCount = DEFAULT_EXECUTOR_COUNT);

        /**
         * Destructor method
         */
        ~TMQDispatcher();

        /**
         * The main progress for dispatching message. If require to call this method again, set true as
         * the return value, otherwise, set false as the return. True value returned indicate that the
         * task has been finished, the executor can be sleep or wait for signal to wakeup.
         * @param eid, a TMQId type value to identify an executor.
         * @return bool, a boolean value indicates whether the task has been finished.
         */
        bool OnExecute(TMQId eid);

        /**
         * Set method for max count of the executor. If the count set to zero, it means that there will
         * be no executor, further more, there may be no messages to be dispatched.
         * @param count the count of the max executors, recommended value is the count of CPU on devices.
         * @return void.
         */
        void SetMaxExecutorCount(int count);

        /**
         * Get method for the max count of the executor.
         * @return the max executor count set by SetMaxExecutorCount
         */
        int GetMaxExecutorCount() const;

        /**
         * RunTask a single topic receiver, internal method. With this method, we can release some mutex,
         * which can avoids some block or time-consuming callbacks holding mutex on a long time.
         * @param topic, topic of the running receiver.
         * @param recId, receiver id to run.
         * @param msg, the tmq message to dispatch.
         * @return void, nothing to be returned.
         */
        void OnRunningReceiver(const char *topic, TMQId recId, const TMQMsg &msg);

        /**
         * Override method for Dispatcher, uses to add a topic receiver.
         * @param topic, the topic of the receiver.
         * @param receiver, a pointer to the receiver.
         * @return a TMQId type id for this topic and receiver.
         */
        virtual TMQId AddReceiver(const char *topic, TMQReceiver *receiver);

        /**
         * Override method for Dispatcher, uses to remove a topic receiver.
         * @param receiverId, the receiver id to remove.
         * @return a boolean value indicate whether the remove is success or not.
         */
        virtual bool RemoveReceiver(TMQId receiverId);

        /**
         * Override method for Dispatcher, uses to find a topic receiver.
         * @param receiverId, the receiver id to find.
         * @return a pointer of the receiver, nullptr will be returned if not found.
         */
        virtual TMQReceiver *FindReceiver(TMQId receiverId);

        /**
         * Wakeup dispatcher for sending tmq messages. When there are any messages arrived, You can call
         * this method. The messages will not send immediately, unless the parameter activeSelf is set
         * to true. Usually, Wakeup will send a signal to the executors. Any executor received the
         * signal will be wake up.
         * @param activeSelf, a boolean value indicate whether activate itself as a executor to send
         *  message immediately.
         * @return void.
         */
        virtual void Wakeup(bool activeSelf);

        /**
         * Dispatch topic messages, internal method.
         * @param topic, the topic of the message.
         * @param msg, the tmq message.
         * @return bool, a boolean value indicate whether it is success or not.
         */
        bool dispatchMessage(const char *topic, const TMQMsg &msg);

        /**
         * Get the receivers subscribe a topic.
         * @param topic the topic to be subscribed
         * @param ids, results of the receiver id.
         * @return TMQSize, a TMQSize type value represent the size of the ids(receivers).
         */
        TMQSize GetTopicReceivers(const char *topic, TMQId **ids);

    };

TMQ_NAMESPACE_END


#endif //TMQ_DISPATCHER_H
