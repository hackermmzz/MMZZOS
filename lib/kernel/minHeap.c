#include "minHeap.h"
#include "../user/syscall.h"
#include"../../kernel/interrupt.h"
#include"../../kernel/memory.h"
void MinHeapInit(MinHeap *heap, MinHeapFunc cmp, uint32_t maxCnt)
{
    //分配内存
    uint8_t old=interrupt_status();interrupt_disable();
    heap->buf=syscall_malloc((maxCnt+1)*sizeof(MinHeapNode));
    if(old)interrupt_enable();
    //
    heap->curCnt=0;
    heap->cmp=cmp;
    heap->maxCnt=maxCnt;
}

void MinHeapInsert(MinHeap *heap, MinHeapNode value)
{
    ASSERT(heap->curCnt<heap->maxCnt);//保证不溢出
    //
    uint8_t old=interrupt_status();
    interrupt_disable();
    //
    MinHeapFunc cmp=heap->cmp;
    MinHeapNode*buff=heap->buf;
    int idx=++(heap->curCnt);
    buff[idx]=value;
    int fa;
    MinHeapNode node_t;
    while((fa=MinHeapFa(idx))&&cmp(buff[idx],buff[fa])){
        //如果父节点不为0并且当前节点小于父节点
        node_t=buff[idx];
        buff[idx]=buff[fa];
        buff[fa]=node_t;
        idx=fa;
    }
    //
    if(old)interrupt_enable();
}

MinHeapNode MinHeapPop(MinHeap *heap)
{
    ASSERT(heap->curCnt);
    //
    uint8_t old=interrupt_status();
    interrupt_disable();
    //
    MinHeapNode*buf=heap->buf;
    MinHeapNode ret=buf[1];
    MinHeapFunc cmp=heap->cmp;
    buf[1]=buf[heap->curCnt];
    --(heap->curCnt);
    //
    int idx=1;
    int size=heap->curCnt;
    int left,right;
    MinHeapNode node_t;
    int targetIdx;
    while((left=MinHeapLeft(idx),right=MinHeapRight(idx),left<=size||right<=size)){
        targetIdx=idx;
        if(left<=size&&cmp(buf[left],buf[targetIdx]))targetIdx=left;
        if(right<=size&&cmp(buf[right],buf[targetIdx]))targetIdx=right;
        node_t=buf[idx];
        buf[idx]=buf[targetIdx];
        buf[targetIdx]=node_t;
        idx=targetIdx;
    }
    //
    if(old)interrupt_enable();
    //
    return ret;
}

void MinHeapDestroy(MinHeap *heap)
{
    uint8_t old=interrupt_status();interrupt_disable();
    syscall_free(heap->buf);
    if(old)interrupt_enable();
}
