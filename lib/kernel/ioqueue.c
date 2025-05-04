#include "ioqueue.h"

void ioQueueInit(struct ioQueue *queue)
{
    MutexInit(&(queue->lock));
    queue->head=queue->tail=0;
    queue->producer=queue->consumer=0;
}

bool ioQueueFull(struct ioQueue *queue)
{
    return IOQUEUE_NEXTPOS(queue,queue->head)==queue->tail;
}

bool ioQueueEmpty(struct ioQueue *queue)
{
    return queue->head==queue->tail;
}
uint32_t ioQueueLen(struct ioQueue*queue){
    if(queue->tail<=queue->head)return queue->head-queue->tail;
    return queue->len+queue->head-queue->tail;
}
void ioQueuePut(struct ioQueue *queue, byte data)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //////////////////////////////////////
    while(ioQueueFull(queue)){
        MutexAcquire(&(queue->lock));
        //
        queue->producer=RunningThread();
        ThreadBlock(BLOCKED);
        //
        MutexRelease(&(queue->lock));
    }
    queue->buff[queue->head]=data;
    queue->head=IOQUEUE_NEXTPOS(queue,queue->head);

    if(queue->consumer){
        ThreadUnBlock(queue->consumer);
        queue->consumer=0;
    }
    //////////////////////////////////////
    if(old)interrupt_enable();
}

byte ioQueueGet(struct ioQueue *queue)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //////////////////////////////////////
    while(ioQueueEmpty(queue)){
        MutexAcquire(&(queue->lock));
        //
        queue->consumer=RunningThread();
        ThreadBlock(BLOCKED);
        //
        MutexRelease(&(queue->lock));
    }
    byte dat=queue->buff[queue->tail];
    queue->tail=IOQUEUE_NEXTPOS(queue,queue->tail);

    if(queue->producer){
        ThreadUnBlock(queue->producer);
        queue->producer=0;
    }
    //////////////////////////////////////
    if(old)interrupt_enable();
    return dat;
}
