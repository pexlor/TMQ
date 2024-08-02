//
//  Pair.h
//  Pair
//
//  Created by  on 2022/8/12.
//  Copyright (c)  Tencent. All rights reserved.
//
#ifndef PAIR_H
#define PAIR_H

// This is a class for wrapping the key-value pair. The Key and Value are both defined as class.
template<class Key, class Value>
class Pair {
public:
    // typedef Pair<Key, Value> to self for convenient usage.
    typedef Pair<Key, Value> Self;

public:
    /**
     * Construct a pair with a const key and value.
     * @param key, the reference to the key.
     * @param value, the reference to the value.
     */
    Pair(const Key &key, const Value &value)
            : key(key), value(value) {
    }

    /**
     * Construct a pair with an exist pair.
     * @param pair, another pair to copy.
     */
    Pair(const Pair<Key, Value> &pair) {
        Assign(pair);
    }

    /**
     * Override the operate = to assign another pair to itself.
     * @param pair, an existed pair.
     * @return the reference the this pair.
     */
    Pair<Key, Value> &operator=(const Pair<Key, Value> &pair) {
        Assign(pair);
        return *this;
    }

private:
    /**
     * Assign operation to set the value of this pair.
     * @param self
     */
    void Assign(const Self &self) {
        if (&self != this) {
            key = self.key;
            value = self.value;
        }
    }

public:
    // The key of this pair.
    Key key;
    // The value of this pair.
    Value value;
};


#endif //PAIR_H
