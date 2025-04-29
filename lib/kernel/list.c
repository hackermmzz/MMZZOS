#include "list.h"

void ListInit(struct List *list)
{
    list->head.pre=list->tail.nxt=0;
    list->head.nxt=&(list->tail);
    list->tail.pre=&(list->head);
}

bool ListEmpty(struct List *list)
{
    return list->head.nxt==&(list->tail);
}

void ListPushFront(struct List *list,struct ListNode*node)
{
    bool status=interrupt_status();
    interrupt_disable();
    node->nxt=list->head.nxt;
    node->pre=&list->head;
    node->nxt->pre=node;
    list->head.nxt=node;
    if(status)interrupt_enable();
}

void ListPushBack(struct List *list,struct ListNode*node)
{
    bool status=interrupt_status();
    interrupt_disable();
    node->nxt=&(list->tail);
    node->pre=list->tail.pre;
    list->tail.pre=node;
    node->pre->nxt=node;
    if(status)interrupt_enable();
}

void ListPopFront(struct List *list)
{
    ASSERT(list->head.nxt!=&list->tail);
    ListRemove(list,list->head.nxt);
}

void ListPopBack(struct List *list)
{
    ASSERT(list->head.nxt!=&list->tail);
    ListRemove(list,list->tail.pre);
}

void ListRemove(struct List *list,struct ListNode *node)
{
    bool status=interrupt_status();
    interrupt_disable();
    node->nxt->pre=node->pre;
    node->pre->nxt=node->nxt;
    if(status)interrupt_enable();
}

struct ListNode *ListBack(struct List *list)
{
    bool status=interrupt_status();
   interrupt_disable();
   struct ListNode*tar=0;
   if(list->head.nxt!=&list->tail){
       tar=list->tail.pre;
   }
   if(status)interrupt_enable();
   return tar;
}

struct ListNode *ListFront(struct List *list)
{
    bool status=interrupt_status();
    interrupt_disable();
    struct ListNode*tar=0;
    if(list->head.nxt!=&list->tail){
        tar=list->head.nxt;
    }
    if(status)interrupt_enable();
    return tar;
}

struct ListNode* ListTraversal(struct List* plist,bool (*func) (struct ListNode*,int),int arg)
{
    struct ListNode* elem = plist->head.nxt;
    if(ListEmpty(plist))	return NULL;
    while(elem != &plist->tail)
    {
    	if(func(elem,arg))	return elem;
    	elem = elem->nxt;
    }
    return NULL;
}