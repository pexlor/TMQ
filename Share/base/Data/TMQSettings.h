//
//  TMQSettings.h
//  TMQSettings
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_SETTINGS_H
#define TMQ_SETTINGS_H

#include <stdio.h>
#include "Pair.h"
#include "RbTree.h"
#include "Chars.h"
#include "TMQMutex.h"

TMQ_NAMESPACE

/**
 *  TMQSettings is implemented using RbTree to save key-value pair.
 */

/// Const definitions
// The topic of tmq setting. The tmq message with this topic will be sent to tmq settings as the
// configurations for tmq.
#define TOPIC_SETTINGS              "__SETTINGS__"
// Define the max message(value) length of the setting.
#define TOPIC_SETTINGS_LENGTH       128

/**
 * TMQSettings has a simple RbTree member to save the key-value pair. It can be used as a map, or It
 * can parse settings from tmq messages using Parse method.
 */
    class TMQSettings {
    private:
        // mutex for kvs
        TMQMutex mutex;
        // RbTree member with key and value are both String type.
        RbTree<String, String> kvs;

    public:
        /**
         * Put method to save key and value into the TMQSettings.
         * @param key, a const pointer to the key.
         * @param value, a const pointer to the value.
         */
        void Put(const char *key, const char *value);

        /**
         * Get method for a key.
         * @param key a pointer to the key.
         * @return the value of the key with String type.
         */
        String Get(const char *key);

        /**
         * Remove a key-pair by a key
         * @param key, a pointer to the key.
         */
        void Remove(const char *key);

        /**
         * Parse tmq settings from tmq messages. The topic of the message must be TOPIC_SETTINGS, and
         * the length of the message data should not exceed TOPIC_SETTINGS_LENGTH. The data of a setting
         * message should have format like this:{key}={value}, which is split by char '=' and the left
         * of = will be the key and the right of = will be the value.
         * @param str, a const pointer the string to parse.
         * @param len, the length of the string.
         * @return, a boolean value indicates whether the parse is success or not.
         */
        static bool Parse(const char *str, int len);

        /**
         * A singleton method for the TMQSettings.
         * @return a pointer to the singleton of the TMQSettings.
         */
        static TMQSettings *GetInstance();
    };

TMQ_NAMESPACE_END

#endif //TMQ_SETTINGS_H
