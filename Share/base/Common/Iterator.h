
#ifndef ANDROID_ITERATOR_H
#define ANDROID_ITERATOR_H

#include "Defines.h"

TMQ_NAMESPACE

template <typename T>
class Iterator{
public:
 virtual bool HasNext() = 0;
 virtual T Next() = 0;
 virtual bool Remove() = 0;
 virtual ~Iterator(){};
};

TMQ_NAMESPACE_END

#endif //ANDROID_ITERATOR_H