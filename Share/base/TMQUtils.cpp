#include "TMQUtils.h"

USING_TMQ_NAMESPACE

int TMQUtils::Trim(const char *str, int len, const char **data)
{
    if (str == nullptr || len <= 0)
    {
        return -1;
    }
    int start = 0;
    int end = len - 1;
    for (int i = 0; i < len; ++i)
    {
        if (str[i] == ' ')
        {
            start++;
        }
        else
        {
            break;
        }
    }
    for (int i = len - 1; i > start; ++i)
    {
        if (str[i] == ' ')
        {
            end--;
        }
        else
        {
            break;
        }
    }
    *data = str + start;
    return end - start + 1;
}

bool TMQUtils::ToInt(const char *str, int len, int *res)
{
    // check if valid
    if (str == nullptr || len <= 0)
    {
        return false;
    }
    int sign = 1;
    int start = 0;
    if (str[0] == '-')
    {
        sign = -1;
        start = 1;
    }
    int result = 0;
    for (int i = start; i < len; ++i)
    {
        if (str[i] - '0' < 0 || str[i] - '9' >= 0)
        {
            return false;
        }
        result = result * 10 + (str[i] - '0');
    }
    *res = sign * result;
    return true;
}