#ifndef __STRING_H

#define __STRING_H

#include <cstring>

#include <stdlib.h>

#include "Defines.h"

TMQ_NAMESPACE

#define SIZE_INVALID (unsigned int)(-1)

class String
{

private:
    unsigned int size;

    char *data;

public:
    String() : size(0), data(nullptr) {}

    String(const char *str) : size(0), data(nullptr)

    {

        if (str)

        {

            Assign(str, strlen(str));
        }
    }

    String(const char *str, unsigned int len)

        : size(0), data(nullptr)

    {

        Assign(str, len);
    }

    String(const String &str)

        : size(0), data(nullptr)

    {

        Assign(str.data, str.size);
    }

    String &operator=(const char *s)

    {

        if (s)

        {

            Assign(s, strlen(s));
        }

        return *this;
    }

    String &operator=(const String &str)

    {

        if (&str != this)

        {

            Assign(str.data, str.size);
        }

        return *this;
    }

    bool operator!=(const String &t) const

    {

        return !(*this == t);
    }

    bool operator==(const String &t) const

    {

        if (size == t.size)

        {

            if (size != 0 && data && t.data)

            {

                return memcmp(data, t.data, size) == 0;
            }
        }

        return false;
    }

    bool operator<(const String &t) const

    {

        if (size < t.size)

        {

            return true;
        }

        if (size == t.size && data && t.data)

        {

            return memcmp(data, t.data, size) < 0;
        }

        return false;
    }

    ~String()

    {

        size = 0;

        if (data)

        {

            delete data;

            data = nullptr;
        }
    }

private:
    void Assign(const char *str, unsigned int len)
    {
        if (data)
        {
            delete[] data;

            data = nullptr;

            size = 0;
        }

        if (str == nullptr || len == SIZE_INVALID)

        {

            return;
        }

        data = (char *)calloc(len + 1, sizeof(char));

        memcpy(data, str, len);

        size = len;
    }

public:
    const char *c_str() const { return data ? data : ""; }

    unsigned int Size() const { return size; }

    String &Append(const char *s)

    {

        if (!s)

        {

            return *this;
        }

        return Append(s, strlen(s));
    }

    String &Append(const char *s, unsigned int len)

    {

        if (s && len > 0)

        {

            unsigned int n = size + len;

            char *d = (char *)calloc(n + 1, sizeof(char));

            if (d)

            {

                memcpy(d, data, size);

                memcpy(d + size, s, len);

                delete[] data;

                data = d;

                size = n;
            }
        }

        return *this;
    }

    String &Append(const String &ts)

    {

        return Append(ts.c_str(), ts.Size());
    }

    int Find(const char c) const

    {

        if (size > 0 && data)

        {

            char *p = strchr(data, c);

            if (p)

            {

                return (int)((p - data) / sizeof(char));
            }
        }

        return -1;
    }

    int Find(const char *s) const

    {

        if (size > 0 && data && s)

        {

            char *p = strstr(data, s);

            if (p)

            {

                return (int)((p - data) / sizeof(char));
            }
        }

        return -1;
    }

    String Substr(int start, int end = -1) const

    {

        if (end < 0 || end > size)

        {

            end = (int)size;
        }

        if (size > 0 && start >= 0 && start < end)

        {

            return String(data + start, end - start);
        }

        return "";
    }

    bool Empty() const

    {

        return size == 0;
    }

    char At(int i)

    {

        if (i >= 0 && i < size)

        {

            return data[i];
        }

        return '\0';
    }
};

typedef String String;

TMQ_NAMESPACE_END

#endif //__STRING_H