#ifndef MMZZ_PROCESS_H
#define MMZZ_PROCESS_H
#include "../kernel/memory.h"
#include "../kernel/thread.h"
#include "tss.h"
#include "../kernel/global.h"
////////////////////////////////////
#define USER_VADDR_START 0x8048000
#define USER_PROCESS_PRIORITY 0x1
////////////////////////////////////ps时候返回给进程的结构体信息
struct ProcessInfo{
    pid_t pid;
    pid_t parent;
    char name[THREAD_NAME_MAX_LEN+1];
    enum ThreadStatus status;
    uint32_t totalTicks;
};
////////////////////////////////////
extern void itr_exit();
void PageDirActivate(struct PCB*pcb);//激活页表
void ProcessActivate(struct PCB*pcb);//激活页表，并且设置tss的0号栈
void ProcessExe(void*fileName,char*name);
void* ProcessCreatePage();
#endif