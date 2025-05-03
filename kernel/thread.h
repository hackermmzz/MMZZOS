#ifndef MMZZ_THREAD_H
#define MMZZ_THREAD_H
#include "memory.h"
#include "../lib/kernel/list.h"
#include "../lib/kernel/minHeap.h"
///////////////////////////////
#define MAX_THREAD_CNT 1024   //MMZZOS最大可以运行1024个线程
#define THREAD_NAME_MAX_LEN 16//线程名字最大长度
#define MAX_FILE_CNT_OPEN_PROCESS  8//每个进程可以打开的最多文件数
typedef void(*ThreadFunc)(void*) ;
//////////////////////////////
extern struct List ReadyList;//就绪队列
extern struct List BlockList;//阻塞队列
extern struct List DelayList;//延迟队列
extern struct List ThreadList;//所有线程都会在这里面
extern struct MinHeap DelayMinHeap;//延迟队列最小堆
extern struct PCB*idle_task;//闲置任务,保证每时每刻都会有任务运行
//////////////////////////////
enum ThreadStatus{
    RUNNING,//运行态
    BLOCKED,//阻塞态
    HANGING,//挂起态
    WAITING,//进程等待子进程退出
    DELAYED,//延时
    READY,//就绪态
    DIED,//死亡
};
struct InterruptStack{//中断栈,产生中断后,进入到中断处理函数前的栈内存布局
    uint32_t code;//中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_;//没用,只是pushad会压入
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    //
    uint32_t error_code;//错误码
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};
struct ThreadStack{//线程栈
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    uint32_t eip;
    uint32_t retAddr;//仅起到占位作用
    ThreadFunc func;
    void*argument;
};

struct PCB{//程序控制块
    uint32_t kernelStack;//内核栈
    pid_t pid;//进程的pid
    pid_t pa_pid;//父进程pid
    enum ThreadStatus status;
    uint8_t priority;
    char name[THREAD_NAME_MAX_LEN+1];
    uint8_t ticks;//当前剩余得滴答数
    uint32_t totalTicks;//总共运行得滴答数
    struct ListNode tagS;//根据当前状态决定tagS在哪个队列
    struct ListNode tag;//在全局队列里面
    struct ListNode tagForSemaphore;//为信号量使用的tag
    uint32_t pageaddr;//进程页表所在虚拟地址
    struct Vaddr_Pool vpool;//用户进程的虚拟内存池
    struct Memory_Block_Desc user_MBD[Memory_Block_Desc_CNT];//用于小内存分配
    uint32_t TargetDelayTime;//延迟到的目标时间点
    uint32_t stackMagic;//用于检测栈溢出
    int32_t fd[MAX_FILE_CNT_OPEN_PROCESS];//
    struct Dir* workDir;//工作所在目录的inode编号
    int8_t exit_status;//退出状态
};
//信号量
struct Semaphore{
    uint8_t value;
    struct List wait;
};
struct Mutex{
    struct Semaphore sem;//0 1信号量实现
    struct PCB*holder;//当前锁的持有者
};
void SemaphoreInit(struct Semaphore*sem,uint8_t value);
void SemaphoreInc(struct Semaphore*sem);
void SemaphoreDec(struct Semaphore*sem);
void MutexInit(struct Mutex*mutex);
void MutexAcquire(struct Mutex*mutex);
void MutexRelease(struct Mutex*mutex);
extern void SwitchTo(struct PCB*cur,struct PCB*nxt);//调度
struct PCB*RunningThread();
void KernelThread(void (func)(void*),void*arg);//将来创建线程通过此函数运行线程函数
void ThreadInit(struct PCB*pcb,const char*name,uint8_t priority);
struct PCB* ThreadCreate(const char*name,uint8_t priority,ThreadFunc func,void*arg);//创建一个线程
uint8_t AllocateTicks(uint8_t priority);//根据优先级分配滴答数
void InitKernalThread();//初始化内核主线程
void init_thread();//初始化线程环境
void schedule(); //对线程进行调度
void ThreadBlock(enum ThreadStatus status);//阻塞线程
void Thread_Yield();//直接调度线程,当然只能给内核线程使用哈
void ThreadUnBlock(struct PCB*pcb);//解除阻塞状态
pid_t PidAllocate();
void PidRecycle(pid_t pid);
void SleepWithoutHang(uint32_t msecond);//休眠函数,与sleep区别是,他并不会被放入最小堆，就是单纯的调度线程
void ThreadExit(struct PCB*pcb,bool needSchuedule);//回收线程的页表和pcb,并且从所有线程队列中移除线程
//////////////////////////////
#endif