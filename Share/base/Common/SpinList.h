#include "Atomic.h"
#define HIGH16(v)   (short)(v >> 16)
#define LOW16(v)    (short)(v)
#define STATE(h, l) (h << 16) | l
template <typename T>
class SpinNode{
public:
    T value;
    SpinNode* volatile next;
    volatile State state;
public:
    SpinNode(): next(nullptr), state(0){};
    SpinNode(const T t): next(nullptr), value(t), state(0){};
};
template <typename T>
class SpinQueue{
public:
    SpinNode<T>* header;
    SpinNode<T>* volatile tail;
    volatile int counter;
    volatile int size;
public:
    SpinQueue():tail(nullptr)
    {
        header = new SpinNode<T>();
        tail = header;
        size = 0;
    }
    void Enqueue(const T& t)
    {
        auto *node = new SpinNode<T>(t);
        // fight for tail
        SpinNode<T> *ptr = tail;
        while (!compare_and_set_strong(&tail, &ptr, &node));
        SpinNode<T> *local = nullptr;
        // connect to old tail
        ptr->next = node;
//        while (!compare_and_set_strong(&(ptr->next), &local, &node));
        add_and_fetch(&size, 1);
    }
};
template <typename T>
class SpinIterator{
private:
    SpinQueue<T>* queue;
    int counter;
    SpinNode<T>* ptr;
private:
    bool Enter(SpinNode<T>* node)
    {
        if (node == nullptr)
        {
            return false;
        }
        State state;
        State expect;
        do {
            do {
                state = node->state;
            } while (HIGH16(state) > 0);
            expect = state + 1;
        } while (!compare_and_set(&(node->state), &state, &expect) );
        return true;
    }
    bool Leave(SpinNode<T> *node)
    {
        State state = node->state;
        State expect;
        do {
            expect = state - 1;
        }
        while (!compare_and_set(&(node->state), &state, &expect));
        return true;
    }
    bool Lift(SpinNode<T> *node)
    {
        State state = node->state & 0x0000ffff;
        State expect = (state | 0x00010000) - 1;
        if (compare_and_set(&(node->state), &state, &expect))
        {
            while (node->state != 0x00010000 || node->next->state != 0);
            return true;
        }
        return false;
    }
    bool Fall(SpinNode<T> *node)
    {
        State state = node->state;
        State expect = 0;
        while (!compare_and_set(&(node->state), &state, &expect));
        return true;
    }
public:
    SpinIterator(SpinQueue<T>* queue): counter(0), ptr(nullptr)
    {
        this->queue = queue;
    }
    virtual bool OnCompare(const T& t)
    {
        return true;
    }
    bool Lookup(T& value)
    {
//        if (counter > 0 && counter != queue->counter)
//        {
//            ptr = nullptr;
//            counter = 0;
//        }
//        if (ptr == nullptr)
//        {
            ptr = queue->header;
//        }
        Enter(ptr);
        SpinNode<T>* found = nullptr;
        while (true)
        {
            if (ptr->next && OnCompare(ptr->next->value) && Lift(ptr))
            {
                found = ptr->next;
                SpinNode<T>* local = found;
                if (found == queue->tail)
                {
                    if (compare_and_set_strong(&(queue->tail), &found, &ptr))
                    {
                        ptr->next = nullptr;
                    } else{
                        while (found->next == nullptr);
                        ptr->next = found->next;
                    }
                } else{
                    ptr->next = found->next;
                }
//                ptr->next = ptr->next->next;
//                if (ptr->next == nullptr)
//                {
//                    if (!compare_and_set(&(queue->tail), &local, &ptr))
//                    {
//                        while (ptr->next == nullptr)
//                        {
//                            ptr->next = found->next;
//                        }
//                    } else{
//                        if (found->next != nullptr)
//                        {
//                            ptr->next = found->next;
//                        }
//                    }
//                }
                queue->counter = counter;
                Fall(ptr);
                break;
            }
            SpinNode<T>* p = ptr;
            if (Enter(ptr->next))
            {
                ptr = ptr->next;
                Leave(p);
            } else{
                counter++;
                Leave(p);
                break;
            }
        }
        if (found)
        {
            value = found->value;
//            LOG_DEBUG("delete:%p %p %p %d", found, queue->header, queue->tail, queue->size);
            delete found;
            sub_and_fetch(&(queue->size), 1);
            return true;
        }
        return false;
    }
};