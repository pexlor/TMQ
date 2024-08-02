//
// Created by 大老鼠 on 2023/1/27.
//

#ifndef LINK_LIST
#define LINK_LIST

typedef bool (*OnCompare)(void *value);

class ValueCompare{
public:
    virtual int OnCompare(void* value) = 0;
};

template<typename T>
class Node{
public:
    Node<T>* next;
    T value;
    Node(const T& value): next(nullptr), value(value)
    {

    }
    Node()
    {
        next = nullptr;
    }
};

template<typename T>
class LinkList{
public:
    Node<T>* header;
    Node<T>* tail;
    Node<T> empty;
    LinkList(): header(nullptr), tail(nullptr)
    {

    }
    void Enqueue(const T& t)
    {
        Node<T>* node = new Node<T>(t);
        if(tail != nullptr){
            tail->next = node;
        }
        tail = node;
        if(header == nullptr)
        {
            header = node;
        }
    }
    bool Consume(ValueCompare* compare, T& val)
    {
        if(header == nullptr)
        {
            return false;
        }
        Node<T>* node = header;
        Node<T>* last = nullptr;
        while (node)
        {
            if(compare->OnCompare(&(node->value)) == 0)
            {
                if(last)
                {
                    last->next = node->next;
                    if(last->next == nullptr)
                    {
                        tail = last;
                    }
                } else{
                    header = node->next;
                }
                val = node->value;
                if(node == header)
                {
                    header = nullptr;
                }
                if(node == tail)
                {
                    tail = nullptr;
                }
                delete node;
                return true;
            }
            last = node;
            node = node->next;
        }
        return false;
    }
};

#endif //LINK_LIST
