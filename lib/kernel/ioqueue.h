#ifndef MMZZ_IO_QUEUE_H
#define MMZZ_IO_QUEUE_H
#include "../../kernel/thread.h"
//////////////////////////////////
#define IOQUEUE_BUF_SIZE 64 
#define IOQUEUE_NEXTPOS(x) (((x)+1)%IOQUEUE_BUF_SIZE)
//////////////////////////////////
struct ioQueue{
    struct Mutex lock;
    uint32_t buff[IOQUEUE_BUF_SIZE];
    int32_t head,tail;
    struct PCB*producer,*consumer;
};
void ioQueueInit(struct ioQueue*queue);
bool ioQueueFull(struct ioQueue*queue);
bool ioQueueEmpty(struct ioQueue*queue);
void ioQueuePut(struct ioQueue*queue,uint32_t data);
uint32_t ioQueueGet(struct ioQueue*queue);
//////////////////////////////////
#endif