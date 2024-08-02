#ifndef ANDROID_RBTREE_H
#define ANDROID_RBTREE_H

#include "Pair.h"

TMQ_NAMESPACE

template <class Key, class Value>
class RbNode
{
private:
    typedef Pair<Key, Value> PValue;
    typedef RbNode<Key, Value> Self;

public:
    Self *parent;
    Self *left;
    Self *right;
    PValue pair;
    int color;

public:
    RbNode(const PValue &value, int color)
        : pair(value), parent(0), left(0), right(0), color(color)
    {
    }
    RbNode(const Self &node)
        : parent(0), left(0), right(0)
    {
        Assign(node);
    }
    ~RbNode()
    {
        parent = 0;
        left = 0;
        right = 0;
    }
    Self &operator=(const Self &self)
    {
        Assign(self);
        return *this;
    }

private:
    void Assign(const Self &self)
    {
        if (&self != this)
        {
            pair = self.pair;
            color = self.color;
            parent = self.parent;
            left = self.left;
            right = self.right;
        }
    }
};

template <class Key, class Value>
class RbIterator
{
private:
    typedef Pair<Key, Value> PValue;
    typedef RbNode<Key, Value> Node;
    typedef RbIterator<Key, Value> Self;

public:
    Node *node;

public:
    RbIterator() : node(0)
    {
    }
    RbIterator(Node *p)
        : node(p)
    {
    }
    RbIterator(const Self &self)
        : node(0)
    {
        Assign(self);
    }
    Self &operator=(const Self &self)
    {
        Assign(self);
        return *this;
    }
    PValue &operator*() const { return node->pair; }
    PValue *operator->() const { return &(operator*()); }
    bool operator==(const Self &self) const { return node == self.node; }
    bool operator!=(const Self &self) const { return node != self.node; }

    Self &operator++()
    {
        Node *next = GetNextDescendant(node->right);
        if (next != 0)
        {
            node = next;
        }
        else
        {
            node = GetNextAncestor(node);
        }
        return *this;
    }
    Self operator++(int)
    {
        Self tmp = *this;
        ++*this;
        return tmp;
    }
    Self &operator--()
    {
        Node *next = GetPreDescendant(node->left);
        if (next != 0)
        {
            node = next;
        }
        else
        {
            node = GetPreAncestor(node);
        }
        return *this;
    }
    Self operator--(int)
    {
        Self tmp = *this;
        --*this;
        return tmp;
    }

private:
    void Assign(const Self &self)
    {
        if (&self != this)
        {
            node = self.node;
        }
    }
    Node *GetNextDescendant(Node *root)
    {
        Node *n = root;
        while (n && n->left)
        {
            n = n->left;
        }
        return n;
    }
    Node *GetNextAncestor(Node *node)
    {
        Node *n = node;
        while (n)
        {
            if (IsLeft(n))
            {
                return n->parent;
            }
            n = n->parent;
        }
        return 0;
    }

    bool IsLeft(Node *n)
    {
        Node *parent = n->parent;
        if (parent != 0 && parent->left == n)
        {
            return true;
        }
        return false;
    }

    Node *GetPreDescendant(Node *root)
    {
        Node *n = root;
        while (n && n->right)
        {
            n = n->right;
        }
        return n;
    }
    Node *GetPreAncestor(Node *node)
    {
        Node *n = node;
        while (n)
        {
            if (IsRight(n))
            {
                return n->parent;
            }
            n = n->parent;
        }
        return 0;
    }

    bool IsRight(Node *n)
    {
        Node *parent = n->parent;
        if (parent != 0 && parent->right == n)
        {
            return true;
        }
        return false;
    }
};

template <class Key, class Value>
class RbTree
{
public:
    typedef RbNode<Key, Value> Node;
    typedef RbIterator<Key, Value> Iterator;
    typedef RbIterator<Key, Value> ConstIterator;

private:
    Node *root;
    unsigned int size;

    enum NColor
    {
        RED = 0,
        BLACK = 1,
    };

public:
    RbTree()
        : root(0), size(0)
    {
    }
    RbTree(const RbTree<Key, Value> &rbTree)
        : root(0), size(0)
    {
        AssignTree(rbTree);
    }
    RbTree<Key, Value> &operator=(const RbTree<Key, Value> &rbTree)
    {
        AssignTree(rbTree);
        return *this;
    }
    ~RbTree()
    {
        Clear();
    }

public:
    Iterator begin() { return GetMinNode(root); }
    ConstIterator begin() const { return GetMinNode(root); }
    Iterator end() { return 0; }
    ConstIterator end() const { return 0; }

    void Clear();
    void Insert(const Pair<Key, Value> &pair);
    Iterator Find(const Key &key) const
    {
        Node *n = FindNode(key);
        return n;
    }
    void Erase(Iterator it);
    unsigned int Size() const;

private:
    void AssignTree(const RbTree<Key, Value> &rbTree);
    void CopyTree(Node **target, const Node *x);
    Node *CreateNode(const Pair<Key, Value> &pair, int color);
    void DeallocNode(Node *node);
    int InsertNode(Node *node);
    Node *FindNode(const Key &key) const;
    Node *GetMinNode(Node *rn) const;
    Node *GetMaxNode(Node *rn) const;
    void RemoveTree(Node *rn);
    bool IsLeft(Node *node) const;
    void BalanceTree(Node *node, Node *&rn);
    void RotateLeft(Node *node, Node *&rootNode);
    void RotateRight(Node *node, Node *&rootNode);

    Node *RebalanceForErase(Node *node,
                            Node *&rootNode,
                            Node *&leftMost,
                            Node *&rightMost);
};

template <class Key, class Value>
void RbTree<Key, Value>::RotateLeft(Node *node, Node *&rootNode)
{
    Node *parent = node->parent;
    Node *right = node->right;

    // ASSERT(right != 0);

    // Lift right node
    if (parent == 0)
    {
        root = right;
        root->parent = 0;
    }
    else
    {
        if (IsLeft(node))
        {
            parent->left = right;
        }
        else
        {
            parent->right = right;
        }
        right->parent = parent;
    }
    // The left node of the right node adopts the right node as the current node.
    node->right = right->left;
    if (node->right)
    {
        node->right->parent = node;
    }
    // The current node adopts the left node as the right node
    right->left = node;
    node->parent = right;
}

template <class Key, class Value>
void RbTree<Key, Value>::RotateRight(Node *node, Node *&rootNode)
{
    Node *parent = node->parent;
    Node *left = node->left;

    // ASSERT(left != 0);

    // Left node
    if (parent == 0)
    {
        root = left;
        root->parent = 0;
    }
    else
    {
        if (IsLeft(node))
        {
            parent->left = left;
        }
        else
        {
            parent->right = left;
        }
        left->parent = parent;
    }
    // The right node of the left node adopts the left node as the current node.
    node->left = left->right;
    if (node->left)
    {
        node->left->parent = node;
    }

    // The current node is passed to the left node as the right node.
    left->right = node;
    node->parent = left;
}

template <class Key, class Value>
void RbTree<Key, Value>::BalanceTree(Node *node, Node *&rn)
{
    // This part of the code is directly deducted from libstdc ++ in gcc.

    Node *x = node;
    x->color = RED;
    while (x != rn && x->parent->color == RED)
    {
        if (x->parent == x->parent->parent->left)
        {
            Node *y = x->parent->parent->right;
            if (y && y->color == RED)
            {
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            }
            else
            {
                if (x == x->parent->right)
                {
                    x = x->parent;
                    RotateLeft(x, rn);
                }
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                RotateRight(x->parent->parent, rn);
            }
        }
        else
        {
            Node *y = x->parent->parent->left;
            if (y && y->color == RED)
            {
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            }
            else
            {
                if (x == x->parent->left)
                {
                    x = x->parent;
                    RotateRight(x, rn);
                }
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                RotateLeft(x->parent->parent, rn);
            }
        }
    }
    rn->color = BLACK;
}

template <class Key, class Value>
void RbTree<Key, Value>::Insert(const Pair<Key, Value> &pair)
{
    Node *newNode = CreateNode(pair, RED);
    if (newNode == 0)
    {
        // ASSERT(0);
        return;
    }
    if (root == 0)
    {
        root = newNode;
        root->color = BLACK;
        size = 1;
    }
    else
    {
        int err = InsertNode(newNode);
        if (err != 0)
        {
            DeallocNode(newNode);
            return;
        }
        // ASSERT(newNode->left == 0);
        // ASSERT(newNode->right == 0);
        BalanceTree(newNode, root);
        size += 1;
    }
}

template <class Key, class Value>
typename RbTree<Key, Value>::Node *
RbTree<Key, Value>::CreateNode(const Pair<Key, Value> &pair, int color)
{
    Node *node = new Node(pair, color);
    return node;
}

template <class Key, class Value>
void RbTree<Key, Value>::DeallocNode(Node *node)
{
    delete node;
}

template <class Key, class Value>
int RbTree<Key, Value>::InsertNode(Node *node)
{
    Node *n = root;
    while (1)
    {
        if (node->pair.key == n->pair.key)
        {
            return -1;
        }
        else if (node->pair.key < n->pair.key)
        {
            if (n->left == 0)
            {
                n->left = node;
                node->parent = n;
                break;
            }
            n = n->left;
        }
        else
        {
            if (n->right == 0)
            {
                n->right = node;
                node->parent = n;
                break;
            }
            n = n->right;
        }
    }
    return 0;
}

template <class Key, class Value>
typename RbTree<Key, Value>::Node *
RbTree<Key, Value>::FindNode(const Key &key) const
{
    Node *n = root;
    while (n != 0)
    {
        if (key == n->pair.key)
            break;
        else if (key < n->pair.key)
        {
            n = n->left;
        }
        else
        {
            n = n->right;
        }
    }
    return n;
}

template <class Key, class Value>
typename RbTree<Key, Value>::Node *
RbTree<Key, Value>::GetMinNode(Node *rn) const
{
    Node *n = rn;
    while (n && n->left)
    {
        n = n->left;
    }
    return n;
}

template <class Key, class Value>
typename RbTree<Key, Value>::Node *
RbTree<Key, Value>::GetMaxNode(Node *rn) const
{
    Node *n = rn;
    while (n && n->right)
    {
        n = n->right;
    }
    return n;
}

template <class Key, class Value>
void RbTree<Key, Value>::Clear()
{
    RemoveTree(root);
    root = 0;
}

template <class Key, class Value>
void RbTree<Key, Value>::RemoveTree(Node *rn)
{
    if (rn == 0)
    {
        return;
    }

    Node *left = rn->left;
    Node *right = rn->right;

    DeallocNode(rn);
    size -= 1;

    if (left != 0)
    {
        RemoveTree(left);
    }
    if (right != 0)
    {
        RemoveTree(right);
    }
}

template <class Key, class Value>
void RbTree<Key, Value>::CopyTree(Node **target, const Node *x)
{
    if (x == 0)
        return;
    Node *node = CreateNode(x->pair, x->color);
    if (node == 0)
    {
        return;
    }

    *target = node;

    if (x->left)
    {
        CopyTree(&node->left, x->left);
        node->left->parent = node;
    }
    if (x->right)
    {
        CopyTree(&node->right, x->right);
        node->right->parent = node;
    }
}

template <class Key, class Value>
typename RbTree<Key, Value>::Node *
RbTree<Key, Value>::RebalanceForErase(Node *node,
                                      Node *&rootNode,
                                      Node *&leftMost,
                                      Node *&rightMost)
{
    // This part of code is directly deducted from libstdc ++ in gcc.

    Node *y = node;
    Node *x = 0;
    Node *parent = 0;
    int tmp_color = 0;
    if (y->left == 0)       // node has at most one non-null child. y == z.
        x = y->right;       // x might be null.
    else if (y->right == 0) // node has exactly one non-null child. y == z.
        x = y->left;        // x is not null.
    else
    {                 // node has two non-null children. Set y to
        y = y->right; // node's successor. x might be null.
        while (y->left != 0)
            y = y->left;
        x = y->right;
    }
    if (y != node)
    { // relink y in place of z. y is z's successor
        node->left->parent = y;
        y->left = node->left;
        if (y != node->right)
        {
            parent = y->parent;
            if (x)
                x->parent = y->parent;
            y->parent->left = x; // y must be a child of left
            y->right = node->right;
            node->right->parent = y;
        }
        else
            parent = y;
        if (rootNode == node)
            rootNode = y;
        else if (node->parent->left == node)
            node->parent->left = y;
        else
            node->parent->right = y;
        y->parent = node->parent;
        tmp_color = node->color;
        node->color = y->color;
        y->color = tmp_color;
        y = node;
        // y now points to node to be actually deleted
    }
    else
    { // y == node
        parent = y->parent;
        if (x)
            x->parent = y->parent;
        if (rootNode == node)
            rootNode = x;
        else if (node->parent->left == node)
            node->parent->left = x;
        else
            node->parent->right = x;
        if (leftMost == node)
        {
            if (node->right == 0) // node->left must be null also
                leftMost = node->parent;
            // makes leftMost == _M_header if node == rootNode
            else
                leftMost = GetMinNode(x);
        }
        if (rightMost == node)
        {
            if (node->left == 0) // node->right must be null also
                rightMost = node->parent;
            // makes rightMost == _M_header if node == rootNode
            else // x == node->left
                rightMost = GetMaxNode(x);
        }
    }
    if (y->color != RED)
    {
        while (x != rootNode && (x == 0 || x->color == BLACK))
            if (x == parent->left)
            {
                Node *w = parent->right;
                if (w->color == RED)
                {
                    w->color = BLACK;
                    parent->color = RED;
                    RotateLeft(parent, rootNode);
                    w = parent->right;
                }
                if ((w->left == 0 ||
                     w->left->color == BLACK) &&
                    (w->right == 0 ||
                     w->right->color == BLACK))
                {
                    w->color = RED;
                    x = parent;
                    parent = parent->parent;
                }
                else
                {
                    if (w->right == 0 ||
                        w->right->color == BLACK)
                    {
                        if (w->left)
                            w->left->color = BLACK;
                        w->color = RED;
                        RotateRight(w, rootNode);
                        w = parent->right;
                    }
                    w->color = parent->color;
                    parent->color = BLACK;
                    if (w->right)
                        w->right->color = BLACK;
                    RotateLeft(parent, rootNode);
                    break;
                }
            }
            else
            { // same as above, with right <-> left.
                Node *w = parent->left;
                if (w->color == RED)
                {
                    w->color = BLACK;
                    parent->color = RED;
                    RotateRight(parent, rootNode);
                    w = parent->left;
                }
                if ((w->right == 0 ||
                     w->right->color == BLACK) &&
                    (w->left == 0 ||
                     w->left->color == BLACK))
                {
                    w->color = RED;
                    x = parent;
                    parent = parent->parent;
                }
                else
                {
                    if (w->left == 0 ||
                        w->left->color == BLACK)
                    {
                        if (w->right)
                            w->right->color = BLACK;
                        w->color = RED;
                        RotateLeft(w, rootNode);
                        w = parent->left;
                    }
                    w->color = parent->color;
                    parent->color = BLACK;
                    if (w->left)
                        w->left->color = BLACK;
                    RotateRight(parent, rootNode);
                    break;
                }
            }
        if (x)
            x->color = BLACK;
    }
    return y;
}

template <class Key, class Value>
void RbTree<Key, Value>::Erase(Iterator it)
{
    Node *node = it.node;
    if (node == 0)
        return;

    Node *most_left = GetMinNode(root);
    Node *most_right = GetMaxNode(root);

    RebalanceForErase(node, root, most_left, most_right);
    DeallocNode(node);
    size -= 1;
}

template <class Key, class Value>
bool RbTree<Key, Value>::IsLeft(Node *node) const
{
    Node *parent = node->parent;
    if (parent != 0 && parent->left == node)
    {
        return true;
    }
    return false;
}

template <class Key, class Value>
void RbTree<Key, Value>::AssignTree(const RbTree<Key, Value> &rbTree)
{
    if (&rbTree != this)
    {
        Clear();
        CopyTree(&root, rbTree.root);
        size = rbTree.size;
    }
}

template <class T1, class T2>
unsigned int RbTree<T1, T2>::Size() const
{
    return size;
}

TMQ_NAMESPACE_END

#endif // ANDROID_RBTREE_H