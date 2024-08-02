#ifndef ANDROID_PAIR_H
#define ANDROID_PAIR_H

#include "Defines.h"

TMQ_NAMESPACE

template <class Key, class Value>
class Pair
{
public:
 typedef Pair<Key, Value> Self;

public:
 Pair(const Key& key, const Value& value)
 : key(key), value(value)
 {
 }
 Pair(const Pair<Key, Value>& pair)
 {
 Assign(pair);
 }
 Pair<Key, Value>& operator=(const Pair<Key, Value>& pair)
 {
 Assign(pair);
 return *this;
 }

private:
 void Assign(const Self& self)
 {
 if (&self != this)
 {
 key = self.key;
 value = self.value;
 }
 }
public:
 Key key;
 Value value;
};

TMQ_NAMESPACE_END

#endif //ANDROID_PAIR_H