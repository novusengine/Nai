#pragma once
#include "pch/Build.h"

#define ListGetStructPtr(node, type, member) \
    reinterpret_cast<type*>(reinterpret_cast<u8*>(node) - offsetof(type, member))

// Defines for iterating lists
#define ListIterate(list, node) \
    for (node = (list)->next; node != (list); node = node->next)

#define ListIterateReverse(list, node) \
    for (node = (list)->prev; node != (list); node = node->prev)

struct List
{
    List* prev = nullptr;
    List* next = nullptr;

    void Init()
    {
        prev = this;
        next = this;
    }

    static void AddNode(List* node, List* prev, List* next)
    {
        prev->next = node;
        next->prev = node;

        node->next = next;
        node->prev = prev;
    }

    static void RemoveNode(List* prev, List* next)
    {
        prev->next = next;
        next->prev = prev;
    }

    static void AddNodeFront(List* list, List* node)
    {
        AddNode(node, list, list->next);
    }
    static void AddNodeBack(List* list, List* node)
    {
        AddNode(node, list->prev, list);
    }

    static u32 GetSize(List* list)
    {
        u32 size = 0;

        List* node;
        ListIterate(list, node)
        {
            size++;
        }

        return size;
    }
};
typedef List ListNode;
