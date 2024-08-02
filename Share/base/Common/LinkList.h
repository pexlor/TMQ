
#ifndef ANDROID_LINKLIST_H
#define ANDROID_LINKLIST_H

#include "Defines.h"

TMQ_NAMESPACE

template <typename T>
class LinkNode{
public:
 T obj;
 LinkNode* next;
public:
 LinkNode(const T& t): obj(t), next(nullptr){};
};

template <typename T>
class LinkIterator{
public:
 LinkNode<T>* node;
 LinkIterator():node(nullptr){};
 LinkIterator(LinkNode<T>* linkNode):node(linkNode){};
 LinkIterator(const LinkIterator& iterator): node(iterator.node){};

 LinkIterator& operator++()
 {
 node = node->next;
 return *this;
 }
 LinkIterator operator++(int)
 {
 LinkIterator tmp = *this;
 ++*this;
 return tmp;
 }
 LinkIterator& operator=(const LinkIterator& iterator)
 {
 node = iterator.node;
 return *this;
 }
 bool operator==(const LinkIterator& iterator) const { return node == iterator.node; }
 bool operator!=(const LinkIterator& iterator)
 {
 return node != iterator.node;
 }
};

template <typename T>
class LinkList{
private:
 LinkNode<T> *header;
 LinkNode<T> *tail;
 int size;
public:
 LinkList(): header(nullptr), tail(nullptr), size(0){};
 LinkNode<T>* Add(const T& t);
 LinkIterator<T> Find(const T& t);
 bool Erase(LinkIterator<T> iterator, LinkIterator<T> lastIterator);
 int Size();

 LinkIterator<T> Begin()
 {
 if (header)
 {
 return header;
 }
 return 0;
 }
 LinkIterator<T> End(){return 0;};
};

template<typename T>
LinkNode<T>* LinkList<T>::Add(const T &t)
{
 LinkNode<T>* node = new LinkNode<T>(t);
 if(!header)
 {
 header = node;
 }
 if(tail)
 {
 tail->next = node;
 }
 tail = node;
 size++;
 return node;
}

template<typename T>
bool LinkList<T>::Erase(LinkIterator<T> iterator, LinkIterator<T> lastIterator){
 if (iterator == End())
 {
 return false;
 }
 // header node
 if (iterator.node == header)
 {
 header = iterator.node->next;
 }
 if (iterator.node == tail)
 {
 tail = lastIterator.node;
 }
 if (lastIterator != End())
 {
 lastIterator.node->next = iterator.node->next;
 }
 delete iterator.node;
 iterator.node = nullptr;
 size--;
 return true;
}

template<typename T>
int LinkList<T>::Size()
{
 return size;
}

template<typename T>
LinkIterator<T> LinkList<T>::Find(const T &t)
{
 LinkIterator<T> iterator = Begin();
 while (iterator != End())
 {
 if(iterator.node->obj == t)
 {
 break;
 }
 iterator ++;
 }
 return iterator;
}

TMQ_NAMESPACE_END

#endif //ANDROID_LINKLIST_H