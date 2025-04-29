#ifndef MMZZ_LIST_H
#define MMZZ_LIST_H
#include "../stdint.h"
#include "../../kernel/interrupt.h"
#include "assert.h"
///////////////////////////////
#define ENABLE_LST_DATA  //默认情况下开启吧,不差这点内存
///////////////////////////////
struct ListNode{
    struct ListNode*pre,*nxt;
    #ifdef  ENABLE_LST_DATA //是否开启这个多余的成员
    union{
        void*ptr;
        uint32_t data;
    };
    #endif
};
struct List{
    struct ListNode head,tail;
};
////////////////////////////
void ListInit(struct List*list);
bool ListEmpty(struct List*list);
void ListPushFront(struct List*list,struct ListNode*node);
void ListPushBack(struct List*list,struct ListNode*node);
void ListPopFront(struct List*list);
void ListPopBack(struct List*list);
void ListRemove(struct List*list,struct ListNode*node);
struct ListNode* ListBack(struct List*list);
struct ListNode* ListFront(struct List*list);
struct ListNode* ListTraversal(struct List* plist,bool (*func) (struct ListNode*,int),int arg);
////////////////////////////
#endif