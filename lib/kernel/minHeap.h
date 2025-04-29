#ifndef MMZZ_MINHEAP_H
#define MMZZ_MINHEAP_H
#include"../stdint.h"
/*
最小堆,设计用于内核的各种优化
*/
////////////////////////////////////
#define MinHeapLeft(idx) ((idx)<<1)
#define MinHeapRight(idx) ((idx)<<1|1)
#define MinHeapFa(idx) ((idx)>>1)
////////////////////////////////////
typedef struct  MinHeapNode{
    union{
        void* ptr;
        uint32_t value;
    };
}MinHeapNode;
typedef bool(*MinHeapFunc)(MinHeapNode,MinHeapNode); 
typedef struct MinHeap{
    MinHeapFunc cmp;
    uint32_t maxCnt;
    uint32_t curCnt;
    MinHeapNode*buf;
}MinHeap;
////////////////////////////////////
void MinHeapInit(struct MinHeap*heap,MinHeapFunc cmp,uint32_t maxCnt);
void MinHeapInsert(MinHeap*heap,MinHeapNode value);
MinHeapNode MinHeapPop(MinHeap*heap);
void MinHeapDestroy(struct MinHeap*heap);
////////////////////////////////////
#endif