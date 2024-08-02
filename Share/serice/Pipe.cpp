#include "Pipe.h"
#include "cstring"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include "limits.h"

USING_TMQ_NAMESPACE

Pipe::Pipe(const char *name, bool block) : name(nullptr), block(true), fd(-1)
{
    if (name && strlen(name) > 0)
    {
        int len = strlen(name);
        this->name = new char[len + 1];
        if (this->name)
        {
            strncpy(this->name, name, len);
            this->name[len] = 0;
        }
    }
    this->block = block;
    if (this->name)
    {
        int mr = mkfifo(this->name, 0666);
        if (mr < 0 && errno != EEXIST)
        {
        }
    }
}

Pipe::~Pipe()
{
    delete[] name;
    name = nullptr;
    block = true;
    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }
}

int Pipe::Write(const void *buf, int length)
{
    if (!buf || length <= 0)
    {
        return -1;
    }
    if (fd <= 0)
    {
        int flag = block ? O_WRONLY : O_WRONLY | O_NONBLOCK;
        fd = open(name, flag);
        if (fd <= 0)
        {
            return -1;
        }
    }
    return write(fd, buf, length);
}

int Pipe::Read(void *buf, int length)
{
    if (buf == nullptr || length <= 0)
    {
        return -1;
    }
    int count = 0;
    while (count <= 0)
    {
        if (fd <= 0)
        {
            int flag = block ? O_RDONLY : O_RDONLY | O_NONBLOCK;
            fd = open(name, flag);
            if (fd <= 0)
            {
                return -1;
            }
        }
        count = read(fd, buf, length);
        if (count == 0)
        {
            close(fd);
            fd = -1;
        }
    }
    return count;
}

int Pipe::GetAtomicLength()
{
    return PIPE_BUF;
}
int Pipe::WriteByte(void *buf, unsigned char byte)
{
    ((unsigned char *)buf)[0] = byte;
    return sizeof(unsigned char);
}

int Pipe::ReadByte(void *buf, unsigned char *byte)
{
    *byte = ((unsigned char *)buf)[0];
    return sizeof(unsigned char);
}

int Pipe::WriteInt(void *buf, int value)
{
    ((unsigned char *)buf)[0] = ((unsigned)value & 0xff000000u) >> 24u;
    ((unsigned char *)buf)[1] = ((unsigned)value & 0x00ff0000u) >> 16u;
    ((unsigned char *)buf)[2] = ((unsigned)value & 0x0000ff00u) >> 8u;
    ((unsigned char *)buf)[3] = ((unsigned)value & 0x000000ffu);
    return sizeof(int);
}

int Pipe::ReadInt(void *buf, int *value)
{
    *value = 0;
    *value = *value | ((unsigned char *)buf)[0] << 24u;
    *value = *value | ((unsigned char *)buf)[1] << 16u;
    *value = *value | ((unsigned char *)buf)[2] << 8u;
    *value = *value | ((unsigned char *)buf)[3];
    return sizeof(int);
}

int Pipe::WriteShort(void *buf, short value)
{
    ((unsigned char *)buf)[0] = ((unsigned)value & 0xff00u) >> 8u;
    ((unsigned char *)buf)[1] = ((unsigned)value & 0x00ffu);
    return sizeof(short);
}

int Pipe::ReadShort(void *buf, short *value)
{
    *value = 0;
    *value = *value | ((unsigned char *)buf)[0] << 8u;
    *value = *value | ((unsigned char *)buf)[1];
    return sizeof(short);
}
int Pipe::WriteChars(void *buf, const char *data, int len)
{
    if (data == nullptr || len <= 0 || buf == nullptr)
    {
        return 0;
    }
    memcpy(buf, data, len);
    return len;
}

int Pipe::GetEncodeLength(const unsigned char *src, int srcLen)
{
    if (src == nullptr || srcLen <= 0)
    {
        return -1;
    }
    int dstLen = 0;
    for (int i = 0; i < srcLen; ++i)
    {
        if (src[i] == 0xff || src[i] == 0x00)
        {
            dstLen += 1;
        }
        dstLen += 1;
    }
    return dstLen;
}

int Pipe::Encode(const unsigned char *src, int srcLen, unsigned char **dst)
{
    if (src == nullptr || srcLen <= 0)
    {
        return -1;
    }
    int dstLen = GetEncodeLength(src, srcLen);
    if (dstLen <= 0)
    {
        return -1;
    }
    *dst = new unsigned char[dstLen];
    if (*dst == nullptr)
    {
        return -1;
    }
    int dstIndex = 0;
    for (int i = 0; i < srcLen; ++i)
    {
        if (src[i] == 0xff || src[i] == 0x00)
        {
            (*dst)[dstIndex++] = src[i];
        }
        (*dst)[dstIndex++] = src[i];
    }
    return dstLen;
}

int Pipe::GetDecodeLength(const unsigned char *src, int srcLen)
{
    if (src == nullptr || srcLen <= 0)
    {
        return -1;
    }
    int delimiterLen = 0;
    for (int i = 0; i < srcLen; ++i)
    {
        if (src[i] == 0xff || src[i] == 0x00)
        {
            delimiterLen += 1;
        }
    }
    int dstLen = 0;
    if (delimiterLen % 2 == 0)
    {
        dstLen = srcLen - delimiterLen / 2;
    }
    return dstLen;
}

int Pipe::Decode(const unsigned char *src, int srcLen, unsigned char **dst)
{
    if (src == nullptr || srcLen <= 0)
    {
        return -1;
    }
    int dstLen = GetDecodeLength(src, srcLen);
    if (dstLen <= 0)
    {
        return -1;
    }
    *dst = new unsigned char[dstLen];
    if (*dst == nullptr)
    {
        return -1;
    }
    int dstIndex = 0;
    for (int i = 0; i < srcLen; ++i)
    {
        (*dst)[dstIndex++] = src[i];
        if ((src[i] == 0x00 || src[i] == 0xff) && src[i] == src[i + 1] && i + 1 < srcLen)
        {
            ++i;
        }
    }
    return dstLen;
}
int Pipe::EncodeByte(const unsigned char *src, unsigned char *dst)
{
    if (src == nullptr || dst == nullptr)
    {
        return 0;
    }
    int dstIndex = 0;
    if (*src == 0xff || *src == 0x00)
    {
        dst[dstIndex++] = *src;
    }
    dst[dstIndex++] = *src;
    return dstIndex;
}
int Pipe::DecodeByte(const unsigned char *src, unsigned char *dst)
{
    if (src == nullptr || dst == nullptr)
    {
        return 0;
    }
    int dstIndex = 0;
    if (src[0] == src[1] && (src[0] == 0xff || src[0] == 0x00))
    {
        dst[dstIndex++] = src[0];
    }
    else
    {
        dst[dstIndex++] = src[0];
        dst[dstIndex++] = src[1];
    }
    return dstIndex;
}
int Pipe::PSend(const PMessage &msg)
{
    unsigned char buf[GetAtomicLength()];
    int index = 0;
    index += Pipe::WriteByte(buf, 0xff);
    unsigned char temp[sizeof(PMessage)];
    unsigned int tempIndex = Pipe::WriteByte(temp, msg.type);
    tempIndex += Pipe::WriteShort(temp + tempIndex, (short)msg.sender);
    tempIndex += Pipe::WriteInt(temp + tempIndex, (int)msg.mid);
    for (int i = 0; i < tempIndex; ++i)
    {
        index += Pipe::EncodeByte(temp + i, buf + index);
    }
    for (int i = 0; i < msg.len; ++i)
    {
        index += Pipe::EncodeByte(msg.data + i, buf + index);
    }
    index += Pipe::WriteByte(buf + index, 0x00);
    index += Pipe::WriteByte(buf + index, 0xff);
    index += Pipe::WriteByte(buf + index, 0x00);
    int count = Write(buf, index);
    return count;
}
bool Pipe::PReceive(PMessage &message)
{
    unsigned char buf[GetAtomicLength()];
    unsigned int bufIndex = 0;
    unsigned char rd[2] = {0};
    while (true)
    {
        int count = Read(rd, 1);
        if (count > 0 && rd[0] == 0xff)
        {
            while (Read(rd + 1, 1) <= 0)
                ;
            buf[bufIndex++] = rd[1];
            if (rd[1] != 0xff)
            {
                break;
            }
        }
    }
    while (true)
    {
        if (Read(rd, 1) > 0)
        {
            buf[bufIndex++] = rd[0];
            if (rd[0] == 0x00)
            {
                while (Read(rd + 1, 1) <= 0)
                    ;
                if (rd[1] != 0x00)
                {
                    break;
                }
            }
        }
    }
    if (bufIndex < 7)
    {
        return false;
    }
    int decodeIndex = 0;
    decodeIndex += Pipe::ReadByte(buf, &message.type);
    decodeIndex += Pipe::ReadShort(buf + decodeIndex, (short *)&message.sender);
    decodeIndex += Pipe::ReadInt(buf + decodeIndex, (int *)&message.mid);
    message.Data(buf + decodeIndex, bufIndex - decodeIndex);
    return true;
}

int Pipe::ReceiveMessage(PMessage &message)
{
    static PMessage *longMessages = nullptr;
    PMessage tempMessage;
    if (!PReceive(tempMessage))
    {
        return 0;
    }
    int result = 1;
    if (tempMessage.type == TYPE_LONG_START || tempMessage.type == TYPE_LONG_END)
    {
        if (longMessages == nullptr)
        {
            longMessages = new PMessage(tempMessage);
        }
        else
        {
            PMessage *last = longMessages;
            while (last->next != nullptr)
                last = last->next;
            last->next = new PMessage(tempMessage);
        }
        result = -1;
    }
    if (tempMessage.type == TYPE_LONG_END)
    {
        PMessage *midLongMessages = nullptr, *mid = nullptr;
        PMessage *leftLongMessages = nullptr, *left = nullptr;
        PMessage *pm = longMessages;
        unsigned totalLen = 0;
        while (pm != nullptr)
        {
            if (pm->sender == tempMessage.sender && pm->mid == tempMessage.mid)
            {
                if (midLongMessages == nullptr)
                {
                    midLongMessages = pm;
                }
                else
                {
                    mid->next = pm;
                }
                mid = pm;
                totalLen += mid->len;
            }
            else
            {
                if (leftLongMessages == nullptr)
                {
                    leftLongMessages = pm;
                }
                else
                {
                    left->next = pm;
                }
                left = pm;
            }
            pm = pm->next;
        }
        if (mid)
            mid->next = nullptr;
        if (left)
            left->next = nullptr;
        if (leftLongMessages)
            longMessages = leftLongMessages;
        if (totalLen > 0)
        {
            auto *ld = (unsigned char *)malloc(totalLen);
            unsigned int ldLen = 0;
            mid = midLongMessages;
            while (mid != nullptr)
            {
                if (ld)
                {
                    memcpy(ld + ldLen, mid->data, mid->len);
                    ldLen += mid->len;
                }
                delete mid;
                mid = mid->next;
            }
            message.type = TYPE_MESSAGE;
            message.Data(ld, ldLen);
        }
    }
    return result;
}
bool Pipe::SendMessage(const PMessage &message)
{
    if (message.data == nullptr || message.len <= 0)
    {
        return false;
    }
    int totalLen = GetEncodeLength((unsigned char *)message.data, (int)message.len) + 4;
    if (totalLen > GetAtomicLength())
    {
        unsigned int tokLen = PMessage::GetTokLen(GetAtomicLength());
        unsigned int count = (message.len + tokLen - 1) / tokLen;
        PMessage pm(message.type, message.sender, message.mid);
        for (int i = 0; i < count; ++i)
        {
            pm.type = (i == count - 1) ? TYPE_LONG_END : TYPE_LONG_START;
            pm.len = (i == count - 1) ? (message.len - i * tokLen) : tokLen;
            pm.data = (unsigned char *)message.data + i * tokLen;
            PSend(message);
        }
    }
    else
    {
        PSend(message);
    }
    return true;
}