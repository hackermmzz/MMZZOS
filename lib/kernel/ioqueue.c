#include "ioqueue.h"

void ioQueueInit(struct ioQueue *queue)
{
    MutexInit(&(queue->lock));
    queue->head=queue->tail=0;
    queue->producer=queue->consumer=0;
}

bool ioQueueFull(struct ioQueue *queue)
{
    return IOQUEUE_NEXTPOS(queue->tail)==queue->head;
}

bool ioQueueEmpty(struct ioQueue *queue)
{
    return queue->head==queue->tail;
}

void ioQueuePut(struct ioQueue *queue, uint32_t data)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //////////////////////////////////////
    while(ioQueueFull(queue)){
        MutexAcquire(&(queue->lock));
        //
        queue->producer=RunningThread();
        ThreadBlock();
        //
        MutexRelease(&(queue->lock));
    }
    queue->buff[queue->head]=data;
    queue->head=IOQUEUE_NEXTPOS(queue->head);

    if(queue->consumer){
        ThreadUnBlock(queue->consumer);
        queue->consumer=0;
    }
    //////////////////////////////////////
    if(old)interrupt_enable();
}

uint32_t ioQueueGet(struct ioQueue *queue)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //////////////////////////////////////
    while(ioQueueEmpty(queue)){
        MutexAcquire(&(queue->lock));
        //
        queue->consumer=RunningThread();
        ThreadBlock();
        //
        MutexRelease(&(queue->lock));
    }
    uint32_t dat=queue->buff[queue->tail];
    queue->tail=IOQUEUE_NEXTPOS(queue->tail);

    if(queue->producer){
        ThreadUnBlock(queue->producer);
        queue->producer=0;
    }
    //////////////////////////////////////
    if(old)interrupt_enable();
    return dat;
}
