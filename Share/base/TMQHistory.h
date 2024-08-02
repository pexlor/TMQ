//
//  TMQHistory.h
//  TMQHistory
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_HISTORY_H
#define TMQ_HISTORY_H

#include "History.h"
#include "RCQueue.h"
#include "Shadow.h"
#include "Ordered.h"
#include "RWMutex.h"
#include "Storage.h"
#include "Watcher.h"

/// const definitions
// The max count of tmq message for each topic.
#define HISTORY_TOPIC_MSG_MAX   128

TMQ_NAMESPACE
/**
 * TMQStat a class for count the tmq messages. It can record message count for different topic,
 * using to release the storage.
 * The current mechanism for history message is that limit the message count for each topic.
 * Default message count of each topic is HISTORY_TOPIC_MSG_MAX.
 */
    class TMQStat {
    public:
        // tmq topic
        char topic[TMQ_TOPIC_MAX_LENGTH]{0};
        // the count of the history message.
        TMQSize count;
    public:
        // Default constructor.
        TMQStat() : count(0) {}

        // Construct a TMQStat with a topic.
        TMQStat(const char *topic) : count(0) {
            if (topic) {
                strncpy(this->topic, topic, sizeof(this->topic));
            }
        }
    };

/**
 * This is the class implementing the RCIterator, using to lookup the history queue.
 * The TopicIterator contains a watcher, which can include multiply topics and compare another topic
 * to see if it is contained by the watcher. A special logic in this iterator is that when there are
 * no any topics in the watcher, the compare of lookup will be always true, that means the
 * TopicIterator will consume all of the message in the history queue.
 */
    class TopicIterator : public RCIterator<Shadow> {
    private:
        // A watcher included multiply topics.
        Watcher watcher;
    public:
        /**
         * Constructor for the TopicIterator.
         * @param queue, the pointer to the random access queue.
         * @param topics, the pointer to a topic array.
         * @param len, the length of the topics that will be add into the watcher.
         * @param copy, a boolean value indicate whether to copy the topics into new memory(deep copy).
         */
        TopicIterator(RCQueue<Shadow> *queue, const char **topics, int len, bool copy = false)
                : watcher(topics, len, copy), RCIterator<Shadow>(queue) {

        }

        /**
         * An override method for lookup compare.
         * @param shadow, the tmq message shadow.
         * @return, true if ready to consume this message, otherwise, return false.
         */
        virtual bool OnCompare(const Shadow &shadow) {
            // Special logic for no topics.
            if (watcher.Size() <= 0) {
                return true;
            }
            // Delegate the watcher to do the comparison.
            return watcher.Contains(shadow.topic);
        }
    };

/**
 * An overview iterator, inheriting from the TopicIterator, using to do consume random access queue
 * from the history messages. It is not same with the TopicIterator, OverviewIterator will traverse
 * the whole history messages, collect the topic messages but not consume it (remove from queue).
 */
    class OverviewIterator : public TopicIterator {
    public:
        // Shadow list to be found.
        List<Shadow> founds;
    public:
        /**
         * Constructor for OverviewIterator
         * @param queue, see TopicIterator
         * @param topics, see TopicIterator
         * @param len, see TopicIterator
         * @param copy, see TopicIterator
         */
        OverviewIterator(RCQueue<Shadow> *queue, const char **topics, int len, bool copy = false)
                : TopicIterator(queue, topics, len, copy) {

        }

        // Compare method for traversing.
        virtual bool OnCompare(const Shadow &t) {
            // If the shadow is what we want.
            if (TopicIterator::OnCompare(t)) {
                founds.Add(t);
            }
            // return false always, the traversing will be from queue start to its end.
            return false;
        }
    };

/**
 * TMQHistory is the implementation for interface IHistory. This is the tmq history message,
 * resource constrained. In the TMQHistory, there is a shadowQueue a topicStats and a pointer to the
 * tmq storage where the tmq message saved. The shadowQueue is a fifo random access lock-free queue
 * that saves the shadows of consumed messages. The topicStats is a ordered list for count the
 * amount of each topic. The ordered list is more efficient for searching using a compared function,
 * which is based on a topic string. So, with the ordered list, we can find the TMQStat very
 * quickly. When the messages with the same topic reach the limitation, TMQHistory will start the
 * reduce task to release some tmq message.
 */
    class TMQHistory : public IHistory {
    private:
        // RCQueue for tmq message shadow.
        RCQueue<Shadow> shadowQueue;
        // Read-Write mutex for topicStats list.
        RWMutex statMutex;
        // a ordered list for TMQStat
        Ordered<TMQStat> topicStats;
        // a pointer for tmq storage instance.
        IStorage *storage;

    private:
        /**
         * Internal private method for calculate the statistics, Checking the tmq messages of a topic
         * Whether reach the limitation or not.
         * @param topic, the topic to be check
         * @param count, the exceeded count
         * @return, a new count for these topic messages.
         */
        TMQSize CalStat(const char *topic, int count);

        /**
         * Reduce task for a topic.
         * @param topic, the topic to run this task.
         * @param count, message count that will be release.
         */
        void CalReduce(const char *topic, int count);

    public:
        /**
         * Default constructor for TMQHistory.
         */
        TMQHistory() : storage(nullptr) {}

        /**
         * Construct a TMQHistory with the pointer to the storage.
         * @param storage, a pointer to the storage.
         */
        TMQHistory(IStorage *storage) : storage(storage) {}

        /**
         * Append a history tmq message shadow. This can be called after a message is consumed from tmq.
         * @param shadow, a message shadow
         */
        void Append(Shadow &shadow);

    public:
        /**
         * Override of the virtual method GetHistory
         * @param topic, a topic array pointer to find.
         * @param len, the length of the topic array.
         * @param msg, a pointer to a tmq message pointer, that will be the found results.
         * @return int , the length of the results found by this method.
         */
        TMQSize GetHistory(const char **topic, int len, TMQMsg **msg);
    };

TMQ_NAMESPACE_END

#endif //TMQ_HISTORY_H
