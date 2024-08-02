#ifndef ANDROID_ORDERED_H
#define ANDROID_ORDERED_H

#include "Define.h"
#include "List.h"

TMQ_NAMESPACE

typedef int (*QuickCompare)(void* p1, void* p2);

template <typename T>
class Ordered: public List<T>{
public:
 int Add(const T& t, QuickCompare compare);
 int FindPosition(const T& t, QuickCompare compare);
 int GetPosition(const T& t, QuickCompare compare);
 bool Exist(const T& t, QuickCompare compare);
};

template<typename T>
int Ordered<T>::FindPosition(const T &t, QuickCompare compare)
{
 if (compare == nullptr)
 {
 return List<T>::Size();
 }
 if(List<T>::Size() == 0)
 {
 return -1;
 }
 long start = 0, end = List<T>::Size() - 1, p = -1;
 int cmp;
 T obj;
 while (start <= end)
 {
 p = (start + end) / 2;
 cmp = compare((void *)&t, (void *)&List<T>::Get(p));
 if (cmp > 0)
 {
 start = p + 1;
 p = start;
 }
 else if (cmp < 0)
 {
 end = p - 1;
 }
 else
 {
 break;
 }
 }
 return p;
}

template<typename T>
int Ordered<T>::Add(const T &t, QuickCompare compare)
{
 int position = FindPosition(t, compare);
 if(position < 0)
 {
 position = 0;
 }
 if(position > List<T>::Size())
 {
 position = List<T>::Size();
 }
 List<T>::Insert(position, t);
 return position;
}

template<typename T>
int Ordered<T>::GetPosition(const T &t, QuickCompare compare)
{
 int pos = FindPosition(t, compare);
 if (pos >= 0 && pos < List<T>::Size())
 {
 T& pt = List<T>::Get(pos);
 if (compare((void *)&t, (void *)&List<T>::Get(pos)) == 0)
 {
 return pos;
 }
 }
 return -1;
}

template<typename T>
bool Ordered<T>::Exist(const T &t, QuickCompare compare)
{
 int pos = GetPosition(t, compare);
 return pos >= 0 && pos < List<T>::Size();
}

TMQ_NAMESPACE_END

#endif //ANDROID_ORDERED_H