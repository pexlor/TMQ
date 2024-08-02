#include "TMQueue.h"
#include "Defines.h"

USING_TMQ_NAMESPACE

void TMQueue::Push(TMsg &tmqMsg)
{
 // find priority position in queue
 int prior = tmqMsg.priority;
 if (prior < 0)
 {
 prior = 0;
 }
 if (prior >= PRIORITY_LENGTH)
 {
 prior = PRIORITY_LENGTH - 1;
 }
 mutex.Lock();
 LinkList<TMsg> *linkList = queue[prior];
 if (linkList == nullptr)
 {
 linkList = new LinkList<TMsg>();
 queue[prior] = linkList;
 }
 tmqMsg.id = ++msgId;
 linkList->Add(tmqMsg);
 size++;
 mutex.UnLock();
}

bool TMQueue::Pick(const char **topics, int len, TMsg &tMsg)
{
 bool suc = false;
 mutex.Lock();
 for (int i = PRIORITY_LENGTH - 1; i >= 0 && !suc; --i)
 {
 if (queue[i] == nullptr)
 {
 continue;
 }
 LinkList<TMsg>* linkList = queue[i];
 LinkIterator<TMsg> iterator = linkList->Begin();
 LinkIterator<TMsg> lastIterator = linkList->End();
 while (iterator != linkList->End())
 {
 if (Contains(iterator.node->obj.topic, topics, len))
 {
 tMsg = iterator.node->obj;
 suc = true;
 linkList->Erase(iterator, lastIterator);
 size--;
 break;
 }
 lastIterator = iterator++;
 }
 }
 mutex.UnLock();
 return suc;
}

bool TMQueue::Contains(const char *topic, const char **topics, int len)
{
 if (topic == nullptr)
 {
 return false;
 }
 if (topics == nullptr || len <= 0)
 {
 return false;
 }
 for (int i = 0; i < len; ++i)
 {
 if (strcmp(topics[i], topic) == 0)
 {
 return true;
 }
 }
 return false;
}

bool TMQueue::AllowPoll(const TMsg& msg, const char **excludes, int len)
{
 return !IS_PICK(msg.flag) && !Contains(msg.topic, excludes, len);
}

bool TMQueue::Poll(TMsg &tMsg, const char **excludes, int len)
{
 bool suc = false;
 mutex.Lock();
 for (int i = PRIORITY_LENGTH - 1; i >= 0 && !suc; --i)
 {
 if (queue[i] == nullptr)
 {
 continue;
 }
 LinkList<TMsg>* linkList = queue[i];
 LinkIterator<TMsg> iterator = linkList->Begin();
 LinkIterator<TMsg> lastIterator = linkList->End();
 while (iterator != linkList->End())
 {
 if (AllowPoll(iterator.node->obj, excludes, len))
 {
 tMsg = iterator.node->obj;
 suc = true;
 linkList->Erase(iterator, lastIterator);
 size--;
 break;
 }
 lastIterator = iterator++;
 }
 }
 mutex.UnLock();
 return suc;
}

unsigned long TMQueue::Size() const
{
 return size;
}