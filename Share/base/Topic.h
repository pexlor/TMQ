//
//  Topic.h
//  Topic
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TOPIC_H
#define TOPIC_H

#if _WINDOWS
#include <stdint.h>
#endif

#include "Defines.h"
#include "TMQTopic.h"
#include "TMQMutex.h"
#include "TMQPicker.h"
#include "Dispatcher.h"
#include "RCQueue.h"
#include "Shadow.h"
#include "TMQStorage.h"
#include "TMQHistory.h"

TMQ_NAMESPACE

/**
 * Implementation for the TMQTopic interface. The class includes a dispatcher, a storage, a history
 * and a priority rc queue.
 *
 * It is a manager for coordinate different modules: dispatcher, storage, history and priority rc
 * queues. The rc queues with multiply priorities are the core member for the Topic. We take
 * advantages of the rc queue(RCQueue) to improve the efficiency of message enqueuing and consuming,
 * that is fifo queue with look-free, and consuming on any positions. There are TMQ_PRIORITY_COUNT
 * RCQueues, each RCQueue has its own priority defined by its position of index.
 *
 * Upon a tmq message coming, it will save to storage first, then enqueue its shadow into correct
 * priority queue. At last, notify the dispatcher there is a message came.
 */
    class Topic : public TMQTopic {
    private:
        // A pointer to dispatcher.
        Dispatcher *dispatcher;
        // A pointer to the storage.
        IStorage *storage;
        // A pointer to the history.
        IHistory *history;
        // Priority RCQueues.
        RCQueue<Shadow> priorityQueue[TMQ_PRIORITY_COUNT];

    public:
        /// Public member methods
        /**
         * Default constructor.
         */
        Topic();

        /**
         * Default destructor.
         */
        ~Topic();

        /**
         * Find the RCQueue with the priority.
         * @param priority, the priority of the queue.
         * @return  a pointer to the found RCQueue, nullptr will be returned if not exist.
         */
        RCQueue<Shadow> *FindQueue(int priority);

        /**
         * Get method for the storage pointer.
         * @return a pointer to the storage.
         */
        IStorage *GetStorage();

        /**
         * Get method for the history pointer.
         * @return
         */
        IHistory *GetHistory();
        /// Public override method from base.
        /**
         * Subscribe a topic message with a TMQReceiver
         * @param topic, the topic to bind to the receiver.
         * @param receiver, a pointer to the TMQReceiver.
         * @return long, a long type value for this subscription.
         */
        virtual TMQId Subscribe(const char *topic, TMQReceiver *receiver);

        /**
         * Cancel a subscription using the subscriber id return by Subscribe.
         * @param subscribeId, the subscribe id
         * @return a boolean value indicates whether this call is success or not.
         */
        virtual bool UnSubscribe(TMQId subscribeId);

        /**
         * Find a TMQReceiver by subscribeId
         * @param subscribeId, the subscribeId returned by Subscribe.
         * @return TMQReceiver*, a pointer to the TMQReceiver registered by Subscribe.
         */
        virtual TMQReceiver *FindSubscriber(TMQId subscribeId);

        /**
         * Publish binary data to the topic with a flag.
         * @param topic, the topic to publish.
         * @param data, a pointer to the raw data.
         * @param length, the length of the raw data.
         * @param flag, flag for this message.
         *  TMQ_MSG_TYPE_PICK, a pick only message. These type of message will be obtained by pick only.
         *  TMQ_MSG_TYPE_DISPATCH, a dispatch only message. These type of message will be dispatched only.
         *  TMQ_MSG_TYPE_ALL, a normal message, will be consumed by pick or dispatch only once.
         *  Default value flag is TMQ_MSG_TYPE_ALL.
         * @param priority, message priority, default value is PRIORITY_NORMAL
         * @return TMQMsgId, a long type value represents the id of this published message.
         */
        virtual TMQMsgId Publish(const char *topic, void *data, int length, int flag = 0,
                                 int priority = PRIORITY_NORMAL);

        /**
         * Publish a tmq message with topic and TMQMsg. It is similar to publish a binary data above.
         * @param topic, the topic of this tmq message.
         * @param tmqMsg, the message detail.
         * @return, long, a long type value represents the id of this published message.
         */
        virtual TMQMsgId Publish(const char *topic, const TMQMsg &tmqMsg);

        /**
         * Enable persistent storage for tmq messages. If the enable is true, the parameter file should
         * be valid file path, the file associated with the path must have read/write permissions. If
         * the file is not exist, it will be created. Besides, if the enable is false, and the
         * persistent instance is running, it will clear the persistence and clear its file.
         * @param enable, a boolean value indicate whether the persistent is enable or not.
         * @param file, a pointer to file path, max length limits to 255.
         * @return bool, a boolean value for whether the persistent is enable or not.
         */
        virtual bool EnablePersistent(bool enable, const char *file);

        /**
         * Create a tmq picker by topics and its message consuming type.
         * @param topics, a pointer to the tmq topic pointers.
         * @param len, the length of the topics.
         * @param type, the message consuming type.
         * @return IPicker*, a pointer to the created picker.
         *  Attentions, a picker will hold the priority queues, if it is no longer used, destroy it
         *  timely by DestroyPicker.
         */
        virtual IPicker *CreatePicker(const char **topics, int len, int type);

        /**
         * Destroy and release a picker.
         * @param picker, a pointer to the picker.
         */
        virtual void DestroyPicker(IPicker *picker);

        /**
         * Get method for the history message.
         * @param topics, the topics to search.
         * @param len, the length of the topics.
         * @param values, a pointer to save the results.
         * @return long, a long type value represents the length of the results(values).
         */
        virtual TMQSize GetHistory(const char **topics, int len, TMQMsg **values);
    };

TMQ_NAMESPACE_END

#endif // TOPIC_H
