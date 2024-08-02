//
//  TMQSettings.cpp
//  TMQSettings
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQSettings.h"
#include "TMQUtils.h"

USING_TMQ_NAMESPACE

/*
 * Put a key-value pair.
 */
void TMQSettings::Put(const char *key, const char *value) {
    String mqKey(key);
    String mqVal(value);
    mutex.Lock();
    kvs.Insert(Pair<String, String>(mqKey, mqVal));
    mutex.UnLock();
}

/*
 * Get a key-value pair.
 */
String TMQSettings::Get(const char *key) {
    String value;
    mutex.Lock();
    // Find the key using RbIterator.
    RbIterator<String, String> iterator = kvs.Find(key);
    if (iterator != kvs.end()) {
        // Find success.
        value = iterator->value;
    }
    mutex.UnLock();
    return value;
}

/*
 * Remove a key-value pair.
 */
void TMQSettings::Remove(const char *key) {
    mutex.Lock();
    // Find and Erase.
    kvs.Erase(kvs.Find(key));
    mutex.UnLock();
}

/*
 * Get the singleton instance without mutex. This method has no concurrency control, so it must be
 * initialized in the other singleton, such as Topic.
 */
TMQSettings *TMQSettings::GetInstance() {
    static TMQSettings *tmqSettings = nullptr;
    if (tmqSettings == nullptr) {
        tmqSettings = new TMQSettings();
    }
    return tmqSettings;
}

/*
 * Parse a string data.
 */
bool TMQSettings::Parse(const char *data, int len) {
    // Check the parameters
    if (data == nullptr || len < 0) {
        return false;
    }
    // Find the position of '='
    int pos = 0;
    while (pos < len && (data)[pos++] != '=');
    // If there is no '=' in the string, or position is not valid, we will return.
    if (pos < 1 || pos >= len || pos >= (TOPIC_SETTINGS_LENGTH + TOPIC_SETTINGS_LENGTH)) {
        return false;
    }
    // obtain key and value, trim blank chars.
    const char *key = nullptr, *value = nullptr;
    int keyLen = -1, valueLen = -1;
    keyLen = TMQUtils::Trim(data, pos - 1, &key);
    valueLen = TMQUtils::Trim(data + pos, len - pos, &value);
    if (keyLen <= 0 || valueLen <= 0) {
        return false;
    }
    // put the key-value into settings.
    TMQSettings::GetInstance()->Put(key, value);
    return true;
}

