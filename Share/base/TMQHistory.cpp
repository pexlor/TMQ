//
//  TMQHistory.cpp
//  TMQHistory
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQHistory.h"

USING_TMQ_NAMESPACE

/**
 * Static method for TMQStat compare. This is a function required by order list.
 * @param one, the first element to be compared.
 * @param another, the second element to be compared.
 * @return, a int value of the compare result.
 */
int StatCompare(void *one, void *another) {
    // use topic string compare directly.
    return strcmp(((TMQStat *) one)->topic, ((TMQStat *) another)->topic);
}

/*
 * Get history messages for some topics. This will delegate the OverviewIterator to loop up in the
 * shadowQueue, but not consume them.
 */
TMQSize TMQHistory::GetHistory(const char **topics, int len, TMQMsg **msg) {
    // Create a OverviewIterator variable.
    OverviewIterator overviewIterator(&shadowQueue, topics, len);
    Shadow shadow;
    // OverviewIterator will return false always, so Lookup will return false, but traverse all
    // shadows.
    overviewIterator.Lookup(shadow);
    // Check found results, and read them if founds are not empty..
    if (!overviewIterator.founds.Empty()) {
        *msg = new TMQMsg[overviewIterator.founds.Size()];
        for (int i = 0; i < overviewIterator.founds.Size(); ++i) {
            storage->Read(overviewIterator.founds.Get(i), (*msg)[i]);
        }
    }
    // Return the size of found results.
    return (int) (overviewIterator.founds.Size());
}

/*
 * Append a history message. Upon appending, we will calculate the the message count of one topic.
 * If the length reaches to HISTORY_TOPIC_MSG_MAX, invoke CalReduce task immediately.
 */
void TMQHistory::Append(Shadow &shadow) {
    shadowQueue.Enqueue(shadow);
    TMQSize topicStatSize = CalStat(shadow.topic, 1);
    if (topicStatSize > HISTORY_TOPIC_MSG_MAX) {
        // invoke CalReduce task
        CalReduce(shadow.topic, (int) (topicStatSize - HISTORY_TOPIC_MSG_MAX));
    }
}

/*
 * Calculate the statistics information. Two key action of this process:
 * 1. Find the TMQStat of the topic from the topicStats, create and add a new TMQStat if necessary.
 * 2. Add up the count in TMQStat using CAS.
 */
TMQSize TMQHistory::CalStat(const char *topic, int count) {
    // Construct a TMQStat variable.
    TMQStat tmqStat(topic);
    // Read lock for topicStats
    int res = statMutex.RLock();
    int sp = topicStats.GetPosition(tmqStat, StatCompare);
    statMutex.RUnlock(res);
    // Write lock for topicStats
    statMutex.WLock();
    if (sp < 0) {
        // Add a new topic statistic
        sp = topicStats.Add(tmqStat, StatCompare);
    }
    TMQStat &stat = topicStats.Get(sp);
    TMQSize local = stat.count;
    TMQSize expect;
    // CAS for add up the stat.count.
    do {
        if (count < 0 && local < -count) {
            expect = 0;
        } else {
            expect = local + count;
        }
    } while (!compare_and_set(&(stat.count), &local, &expect));
    statMutex.WUnlock();
    return expect;
}

/*
 * Reduce task for a topic.
 * This will look up at the shadowQueue, find and consume the topic shadows. The consume to the
 * the topic shadows should be limit to count. For example, if the message count with the topic is
 * 10, the count in parameters is 2, so the Lookup should be executed 2, and the result size will
 * be 8 at last.
 */
void TMQHistory::CalReduce(const char *topic, int count) {
    // count is zero, return quickly.
    if (count <= 0) {
        return;
    }
    // The shadow queue iterator.
    TopicIterator topicIterator(&shadowQueue, &topic, 1);
    Shadow shadow;
    int reduceCount = count;
    // Loop for releasing the tmq messages.
    while (reduceCount > 0 && topicIterator.Lookup(shadow)) {
        if (storage) {
            // Remove a message from storage.
            storage->Remove(shadow);
        }
        reduceCount--;
    }
    CalStat(topic, count);
}
