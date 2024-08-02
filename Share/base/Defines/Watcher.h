//
//  Watcher.h
//  Watcher
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef WATCHER_H
#define WATCHER_H

#include "TMQTopic.h"
#include "string.h"
/// Const definitions
// WATCHER_STACK_SIZE is a length of topics that can be saved into the stack.
#define WATCHER_STACK_SIZE      1

/**
 * A watcher class is used to wrap multiply topics, and provide a Contains function to check whether
 * a topic is existed in the watcher or not. Multiply topics with its length is not friendly for
 * coding, and every memory copying of topics are very consuming. So we put the topics into a
 * wrapper class called Watcher. In the Watcher, we will use some small tricks to improve the
 * efficiency.
 */
class Watcher {
private:
    // The length of the topics
    int size;
    // Stack memory for saving a topic string or a pointer to the topics.
    char topics[TMQ_TOPIC_MAX_LENGTH]{0};
    // A boolean value indicates whether copy the topics to new memory or not.
    bool copied;
public:
    /**
     * Default constructor for the Watcher.
     */
    Watcher() : copied(false), size(0) {

    }

    /**
     * Construct a watcher with topics and its length.
     * @param topics, a pointer to the topics.
     * @param len, the count of the topics.
     * @param copied, a boolean value indicate whether to copy the topics or not.
     */
    Watcher(const char **topics, int len, bool copied = true) {
        Assign(topics, len, copied);
    }

    /**
     * Construct a watcher with an existed watcher.
     * @param watcher, an existed watcher.
     */
    Watcher(const Watcher &watcher) {
        Assign((const char **) (watcher.topics), watcher.size, watcher.copied);
    }

    /**
     * Override operator '='
     * @param watcher, the original watcher.
     * @return, a new watcher
     */
    Watcher &operator=(const Watcher watcher) {
        Assign((const char **) (watcher.topics), watcher.size, watcher.copied);
        return *this;
    }

    /**
     * Destructor for the watcher. If the topics is in heap memory, release them.
     */
    ~Watcher() {
        if (size > WATCHER_STACK_SIZE && copied) {
            char **tps = nullptr;
            memcpy(&tps, this->topics, sizeof(tps));
            for (int i = 0; i < size && tps; ++i) {
                delete[] tps[i];
            }
            delete[] tps;
        }
    }

    /**
     * Size of the topics.
     * @return size.
     */
    const int Size() {
        return size;
    }

    /**
     * Assign the topics to the watcher. If the size is WATCHER_STACK_SIZE, the topic will be saved
     * in the stack. If the copied is false, we will use the topics pointer directly. Otherwise, If
     * the copied is true and size is beyond to WATCHER_STACK_SIZE, the topics will be deep copied.
     * @param src, a pointer to the topics.
     * @param len, the count of topics.
     * @param copy, a boolean value indicate whether to copy the topics or not.
     */
    void Assign(const char **src, int len, bool copy) {
        // Only one topic, copy to stack directly.
        if (src && len == WATCHER_STACK_SIZE) {
            strncpy(this->topics, src[0], sizeof(this->topics));
        } else if (src && len > WATCHER_STACK_SIZE) {
            // Deep copy for the topics.
            if (copy) {
                char **tps = new char *[len];
                for (int i = 0; i < len; ++i) {
                    tps[i] = new char[TMQ_TOPIC_MAX_LENGTH];
                    memset(tps[i], 0, TMQ_TOPIC_MAX_LENGTH);
                    strncpy(tps[i], src[i], TMQ_TOPIC_MAX_LENGTH);
                }
                memcpy(this->topics, &tps, sizeof(tps));
            } else {
                // No need deep copy, put the pointer into this->topics.
                memcpy(this->topics, &src, sizeof(src));
            }
        }
        this->copied = copy;
        this->size = len;
    }

    /**
     * Check whether the topic is existed in the watcher.
     * @param topic, a topic to check.
     * @return a boolean value indicates whether the topic is existed in the watcher or not
     */
    bool Contains(const char *topic) {
        // Only one topic.
        if (topic && size == WATCHER_STACK_SIZE) {
            return strcmp(this->topics, topic) == 0;
        }
        if (topic && size > WATCHER_STACK_SIZE) {
            // Multiply topics, compare one by one. If found, stop the loop and return true.
            char **tps = nullptr;
            memcpy(&tps, this->topics, sizeof(tps));
            for (int i = 0; i < size && tps; ++i) {
                if (strcmp(tps[i], topic) == 0) {
                    return true;
                }
            }
        }
        // Find over, but there is no topic included, return false.
        return false;
    }
};

#endif //WATCHER_H
