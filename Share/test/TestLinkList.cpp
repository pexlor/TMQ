#include <cassert>
#include "Defines.h"
#include "LinkList.h"

USING_TMQ_NAMESPACE

void testAdd()
{
    // test add method with add and erase
    LOG_DEBUG("test LinkList, method add.");
    char a = 'a';
    LinkList<char> linkList;
    linkList.Add(a);
    assert(linkList.Size() == 1);
    assert(linkList.Begin().node->obj == a);
    linkList.Erase(linkList.Begin(), linkList.End());
    assert(linkList.Size() == 0);

    // test add method with 1000 element
    int it = 1000;
    LinkList<int> intLinkList;
    for (int i = 0; i < it; ++i)
    {
        intLinkList.Add(i);
    }
    assert(intLinkList.Size() == it);
    for (int i = 0; i < it; ++i)
    {
        intLinkList.Erase(intLinkList.Begin(), intLinkList.End());
    }
    assert(intLinkList.Size() == 0);
    LOG_DEBUG("test LinkList, method add, pass");
}

void testFind()
{
    LOG_DEBUG("test LinkList, method find.");
    char a = 'a';
    LinkList<char> linkList;
    assert(linkList.Find(a) == linkList.End());
    linkList.Add(a);
    assert(linkList.Find(a) == linkList.Begin());
    LOG_DEBUG("test LinkList, method find, pass.");
}

void testLinkList()
{
    LOG_DEBUG("test LinkList, start");
    testAdd();
    testFind();
    LOG_DEBUG("test LinkList, finish");
}