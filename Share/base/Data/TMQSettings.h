#ifndef ANDROID_TMQSETTINGS_H
#define ANDROID_TMQSETTINGS_H

#include "Pair.h"
#include "RbTree.h"
#include "Chars.h"
#include "TMQMutex.h"

TMQ_NAMESPACE

#define TOPIC_SETTINGS_LENGTH 128
#define TMQ_MAX_EXECUTOR_COUNT "MAX_EXECUTOR_COUNT"

class TMQSettings
{
private:
    TMQMutex mutex;
    RbTree<String, String> kvs;

public:
    void Put(const char *key, const char *value);
    String Get(const char *key);
    void Remove(const char *key);
    static bool Parse(const char *str, int len);

    static TMQSettings *GetInstance();
};

TMQ_NAMESPACE_END

#endif // ANDROID_TMQSETTINGS_H