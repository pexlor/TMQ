//
//  Chars.h
//  Chars
//
//  Created by  on 2022/6/18.
//  Copyright (c)  Tencent. All rights reserved.
//
#ifndef TMQ_STRING_H
#define TMQ_STRING_H

#include "Defines.h"
#include <cstring>
#include <stdlib.h>

TMQ_NAMESPACE

/**
 * TMQ String for chars. This class providers a convenient way to use the char array. Since we have
 * no stl template included into this project, we had to implement a String by ourself. Here is the
 * simple implementation, contains most of the common functions, such as constructor, base operator(
 * =, ==, >, etc), append, find, c_str, and so on. The disadvantages of the String implementation is
 * that the memory usage is deep copied for each constructor of the string, which is low efficient.
 * So this String class should be used with no concerns about the efficiency.
 */
    class String {
    private:
        // a pointer to the binary char array.
        char *data;
        // the length of the data.
        TMQSize size;

    private:
        /**
         * Private method to assign data. Before applying for new memory, release the old memory first.
         * @param str
         * @param len
         */
        void Assign(const char *str, TMQSize len) {
            // Release old memory.
            if (data) {
                delete[] data;
                data = nullptr;
                size = 0;
            }
            // Check parameters
            if (str == nullptr || len == SIZE_INVALID) {
                return;
            }
            // Apply new memory, if success, copy str to data with length len.
            data = (char *) calloc(len + 1, sizeof(char));
            if (data) {
                memcpy(data, str, len);
                size = len;
            }
        }

    public:
        /**
         * Default constructor.
         */
        String() : size(0), data(nullptr) {}

        /**
         * Construct a String with char* must ending with '\0'
         * @param str, a const pointer to the source string.
         */
        String(const char *str) : size(0), data(nullptr) {
            // Check valid pointer.
            if (str) {
                Assign(str, strlen(str));
            }
        }

        /**
         * Construct a String with char* and its length.
         * @param str, a pointer to the string.
         * @param len, the length of the str.
         */
        String(const char *str, TMQSize len)
                : size(0), data(nullptr) {
            Assign(str, len);
        }

        /**
         * Construct a new String with an exist one.
         * @param str, the original string.
         */
        String(const String &str)
                : size(0), data(nullptr) {
            Assign(str.data, str.size);
        }

        /**
         * Overload the operator = with const char*
         * @param s, pointer to a const string.
         * @return, reference to this
         */
        String &operator=(const char *s) {
            if (s) {
                Assign(s, strlen(s));
            }
            return *this;
        }

        /**
         * Overload the operator = with const String
         * @param str, a reference to the original string.
         * @return , reference to this
         */
        String &operator=(const String &str) {
            if (&str != this) {
                Assign(str.data, str.size);
            }
            return *this;
        }

        /**
         * Overload the operator != with const String.
         * @param t, the String to be compared.
         * @return bool, a boolean value indicates whether it is not equal or equal.
         */
        bool operator!=(const String &t) const {
            return !(*this == t);
        }

        /**
         * Overload the operator == with const String
         * @param t, the String to be compared.
         * @return a boolean value indicates whether it is equal or not.
         */
        bool operator==(const String &t) const {
            // Check at the size of the String first.
            if (size == t.size) {
                // Check the data valid.
                if (size != 0 && data && t.data) {
                    // Compare with the memcmp with length.
                    return memcmp(data, t.data, size) == 0;
                }
            }
            return false;
        }

        /**
         * Overload the operator < with const String
         * @param t, the String to be compared.
         * @return a boolean value indicates whether it is below or not.
         */
        bool operator<(const String &t) const {
            // Compare the size of String first.
            if (size < t.size) {
                return true;
            }
            // If the size is equal, compare the data at next.
            if (size == t.size && data && t.data) {
                return memcmp(data, t.data, size) < 0;
            }
            return false;
        }

        /**
         * Release this object and its memory.
         */
        ~String() {
            size = 0;
            // if the data pointer is valid, delete it to release the memory.
            if (data) {
                delete data;
                data = nullptr;
            }
        }

        /**
         * Achieve the c style data.
         * @return a pointer to the string.
         */
        const char *c_str() const {
            return data ? data : "";
        }

        /**
         * Get the size of the string.
         * @return TMQSize, the length of the String.
         */
        TMQSize Size() const {
            return size;
        }

        /**
         * Append a sub string to this object.
         * @param s, a pointer to a const string.
         * @return, reference of this String.
         */
        String &Append(const char *s) {
            if (!s) {
                return *this;
            }
            return Append(s, strlen(s));
        }

        /**
         * Append a string pointed by s with the length.
         * @param s, a pointer to a string.
         * @param len, the length to append.
         * @return this.
         */
        String &Append(const char *s, TMQSize len) {
            // Check parameters, execute these method unless s and len are both valid.
            if (s && len > 0) {
                // Calculate the new size: size + len, and assign the result to n.
                TMQSize n = size + len;
                // Apply new memory with n + 1 length.
                char *d = (char *) calloc(n + 1, sizeof(char));
                // If success with the calloc, copy data and s to new memory space.
                if (d) {
                    memcpy(d, data, size);
                    memcpy(d + size, s, len);
                    delete[] data;
                    data = d;
                    size = n;
                }
            }
            return *this;
        }

        /**
         * Append string.
         * @param ts, reference to a String.
         * @return reference to this.
         */
        String &Append(const String &ts) {
            return Append(ts.c_str(), ts.Size());
        }

        /**
         * Find a const char in the string.
         * @param c, a value of the char
         * @return int, a int value indicates the position in the string.
         */
        int Find(const char c) const {
            if (size > 0 && data) {
                // Find the pointer of char c.
                char *p = strchr(data, c);
                if (p) {
                    // Calculate the position of the char c.
                    return (int) ((p - data) / sizeof(char));
                }
            }
            return -1;
        }

        /**
         * Find a sub string from this string.
         * @param s, the string to find.
         * @return the position of the sub string.
         */
        int Find(const char *s) const {
            if (size > 0 && data && s) {
                char *p = strstr(data, s);
                if (p) {
                    return (int) ((p - data) / sizeof(char));
                }
            }
            return -1;
        }

        /**
         * Check the string is empty or not.
         * @return, true if the String is empty, otherwise false will be returned.
         */
        bool Empty() const {
            return size == 0;
        }

        /**
         * Get the char at position i.
         * @param i, the position to find.
         * @return, the char at position i, '0' will be return if there is no char at position i.
         */
        char At(int i) {
            if (i >= 0 && i < size) {
                return data[i];
            }
            return '\0';
        }
    };

TMQ_NAMESPACE_END

#endif //TMQ_STRING_H
