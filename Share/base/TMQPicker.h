//
//  TMQPicker.h
//  TMQHistory
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_PICKER_H
#define TMQ_PICKER_H

#include "Defines.h"
#include "TMQTopic.h"
#include "RCQueue.h"
#include "Shadow.h"
#include "List.h"
#include "Chars.h"
#include "Storage.h"
#include "TMQHistory.h"
#include "Watcher.h"
#include "Topic.h"

TMQ_NAMESPACE

/**
 * A Lookup shadow iterator. This class is used to implement the RCIterator and find the topic
 * messages. A ShadowIterator contains a pointer of watcher and a int value for message type. The
 * Lookup compare will be delegated to the watcher.
 */
    class ShadowIterator : public RCIterator<Shadow> {
    private:
        // A pointer to the topics watcher.
        Watcher *watcher;
        // The message type for pick. This type will do AND operation with the flag of the shadow(msg).
        // If the result by the AND operation is not zero, this shadow will compare the topics at next.
        int type;
    public:
        /**
         * Default constructor for ShadowIterator
         */
        ShadowIterator() : watcher(nullptr), type(0) {

        }

        /**
         * Construct a ShadowIterator with rc queue and topics.
         * @param queue, the pointer to the rc queue
         * @param watch, a pointer to the watcher.
         * @param type, the message type to be consumed.
         */
        ShadowIterator(RCQueue<Shadow> *queue, Watcher *watch, int type)
                : RCIterator<Shadow>(queue), type(type), watcher(watch) {

        }

        /**
         * Overriding method for RCIterator using to do compare.
         * @param t, the shadow of a tmq message.
         * @return, a boolean value indicate whether to consume this message or not.
         */
        bool OnCompare(const Shadow &t) override {
            // If AND operation between flag and type is fail, return false quickly.
            if ((t.flag & type) == 0) {
                return false;
            }
            // Special logic for scene of no topics, this scene will consume all messages. Be careful.
            if (watcher->Size() == 0) {
                return true;
            }
            // Delegate the topic compare to watcher.
            return watcher->Contains(t.topic);
        }
    };

/**
 * TMQ picker is used for picking topic messages. A picker will include a topic watcher, a storage,
 * a history and the priority rc queue iterators.
 * TMQ picker can pick messages with priorities. The message with high priority will be picked
 * faster.
 */
    class TMQPicker : public IPicker {
    private:
        // The type of the picker, indicate what messages can be picked from message queue.
        int type;
        // A watcher that contains one or more topics.
        Watcher topicWatcher;
        // A pointer to the topic message storage.
        IStorage *topicStorage;
        // A pointer to the history messages.
        IHistory *topicHistory;
        // A pointer to the shadow iterator pointer. The length of the iterators is TMQ_PRIORITY_COUNT.
        ShadowIterator *shadowIterators[TMQ_PRIORITY_COUNT]{0};

    public:
        /**
         * Construct a tmq picker with topics and consumed message type.
         * @param topics, a pointer to the topic array pointer.
         * @param len, the count of the topic.
         * @param type, the message type to be consumed.
         */
        TMQPicker(const char **topics, int len, int type);

        /**
         * Set method for modifying the pointer of a storage.
         * @param storage, the pointer to the storage.
         */
        void SetStorage(IStorage *storage);

        /**
         * Set method for history.
         * @param history, a pointer to the history.
         */
        void SetHistory(IHistory *history);

        /**
         * Set method for RCQueues.
         * @param topicQueues, the rc queues with priority.
         */
        void SetQueues(RCQueue<Shadow> *topicQueues);

        /*
         * Destructor the tmq picker.
         */
        ~TMQPicker();

        /**
         * Override method for Pick.
         * @param topic, a pointer to the topic, using to receive the picked topic.
         * @param tmqMsg, a reference to the TMQMsg, using to receive the tmq message.
         * @return bool, a boolean value indicate whether we have picked a message or not.
         */
        virtual bool Pick(char *topic, TMQMsg &tmqMsg);
    };

TMQ_NAMESPACE_END

#endif //TMQ_PICKER_H
