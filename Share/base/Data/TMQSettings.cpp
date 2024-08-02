#include "TMQSettings.h"
#include "TMQUtils.h"

USING_TMQ_NAMESPACE

void TMQSettings::Put(const char *key, const char *value)
{
    String mqKey(key);
    String mqVal(value);
    mutex.Lock();
    kvs.Insert(Pair<String, String>(mqKey, mqVal));
    mutex.UnLock();
}

String TMQSettings::Get(const char *key)
{
    String value;
    mutex.Lock();
    RbIterator<String, String> iterator = kvs.Find(key);
    if (iterator != kvs.end())
    {
        value = iterator->value;
    }
    mutex.UnLock();
    return value;
}

void TMQSettings::Remove(const char *key)
{
    mutex.Lock();
    kvs.Erase(kvs.Find(key));
    mutex.UnLock();
}

TMQSettings *TMQSettings::GetInstance()
{
    static TMQSettings *tmqSettings = nullptr;
    if (tmqSettings == nullptr)
    {
        tmqSettings = new TMQSettings();
    }
    return tmqSettings;
}

bool TMQSettings::Parse(const char *data, int len)
{
    if (data == nullptr || len < 0)
    {
        return false;
    }
    int pos = 0;
    while (pos < len && (data)[pos++] != '=')
        ;
    if (pos < 1 || pos >= len || pos >= (TOPIC_SETTINGS_LENGTH + TOPIC_SETTINGS_LENGTH))
    {
        return false;
    }
    // obtain key and value, trim blank char.
    const char *key = nullptr, *value = nullptr;
    int keyLen = -1, valueLen = -1;
    keyLen = TMQUtils::Trim(data, pos - 1, &key);
    valueLen = TMQUtils::Trim(data + pos, len - pos, &value);
    if (keyLen <= 0 || valueLen <= 0)
    {
        return false;
    }
    TMQSettings::GetInstance()->Put(key, value);
    return true;
}