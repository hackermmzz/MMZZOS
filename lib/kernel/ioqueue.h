#ifndef MMZZ_IO_QUEUE_H
#define MMZZ_IO_QUEUE_H
#include "../../kernel/thread.h"
//////////////////////////////////
#define IOQUEUE_NEXTPOS(queue,x) (((x)+1)%(queue->len))
//////////////////////////////////
struct ioQueue{
    struct Mutex lock;
    uint32_t len;
    byte*buff;
    int32_t head,tail;
    struct PCB*producer,*consumer;
};
void ioQueueInit(struct ioQueue*queue);
bool ioQueueFull(struct ioQueue*queue);
bool ioQueueEmpty(struct ioQueue*queue);
void ioQueuePut(struct ioQueue*queue,byte data);
byte ioQueueGet(struct ioQueue*queue);
uint32_t ioQueueLen(struct ioQueue*queue);
//////////////////////////////////
#endif