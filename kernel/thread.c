#include "thread.h"
#include "../userprog/process.h"
#include "../lib/kernel/print.h"
#include "../fs/dir.h"
#define MemberOffset(structType,member) ((uint32_t)(&(((structType*)0)->member))) //返回成员在结构体里面的偏移量
#define DelayMinHeapMaxCnt 1024//默认最大只能同时有1024个延迟任务
/////////////////////////////////////
uint32_t systemTicks;
struct PCB*mainThread;//当前执行得主线程
struct List ReadyList;//就绪队列
struct List BlockList;//阻塞队列
struct List DelayList;//延迟队列
struct List ThreadList;//所有线程都会在这里面
struct Mutex pidLock;//给进程分配pid需要上锁
struct MinHeap DelayMinHeap;//延迟队列最小堆
struct PCB*idle_task;//闲置任务,保证每时每刻都会有任务运行
void SemaphoreInit(struct Semaphore *sem, uint8_t value)
{
    sem->value=value;
    ListInit(&(sem->wait));
}
void SemaphoreInc(struct Semaphore *sem)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    ASSERT((sem->value)<255);//防止溢出
    if(!ListEmpty(&(sem->wait))){
        struct ListNode*tag=ListFront(&(sem->wait));
        ListRemove(&(sem->wait),tag);//从等待队列移除
        struct PCB*pcb=(struct PCB*)(((uint32_t)tag)-MemberOffset(struct PCB,tagForSemaphore));
        ThreadUnBlock(pcb);//解除阻塞
    }
    sem->value++;
    if(old)interrupt_enable();
}
void SemaphoreDec(struct Semaphore *sem)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    while(sem->value==0){//信号量为0,阻塞当前线程
        struct PCB*cur=RunningThread();
        ListPushBack(&(sem->wait),&(cur->tagForSemaphore));
        ThreadBlock();//阻塞当前线程
    }
    sem->value--;
    if(old)interrupt_enable();
}
void MutexInit(struct Mutex *mutex)
{
    mutex->holder=0;
    SemaphoreInit(&(mutex->sem),1);
}
void MutexAcquire(struct Mutex *mutex)
{
    SemaphoreDec(&(mutex->sem));
    mutex->holder=RunningThread();
}
void MutexRelease(struct Mutex *mutex)
{
    mutex->holder=0;
    SemaphoreInc(&(mutex->sem));
}
/////////////////////////////////////
struct PCB*RunningThread(){
    uint32_t esp;
    asm volatile("movl %%esp,%0":"=g"(esp));
    return (struct PCB*)(esp&0xfffff000);
}
void KernelThread(void(func)(void *), void *arg)
{
    interrupt_enable();//开启中断
    func(arg);
}

void ThreadInit(struct PCB *pcb, const char *name, uint8_t priority)
{ 
    memset(pcb,0,sizeof(struct PCB));
    strcpy(pcb->name,name);
    pcb->priority=priority;
    pcb->stackMagic=1190650;
    pcb->pageaddr=0;
    pcb->ticks=AllocateTicks(pcb->priority);//分配滴答数
    pcb->pid=PidAllocate();//获取进程pid
    pcb->totalTicks=0;
    pcb->status=READY;//线程初始化时默认状态为就绪态,也就是说创建完毕后就可以执行了
    pcb->workDir=&rootDir;//默认为根目录
    for(int i=0;i<MAX_FILE_CNT_OPEN_PROCESS;++i)pcb->fd[i]=-1;//初始化文件描述符
    uint32_t esp=((uint32_t)(pcb))+PAGE_SIZE;//栈顶
    pcb->kernelStack=esp-sizeof(struct InterruptStack)-sizeof(struct ThreadStack);//减去中断栈和线程栈就是内核栈
}

struct PCB* ThreadCreate(const char *name, uint8_t priority, ThreadFunc func, void *arg)
{
    //
    struct PCB*pcb=mallocKernelPage(1);//所有pcb都在内核空间,pcb独占一页内存
    ASSERT(pcb!=0);//
    //初始化pcb
    ThreadInit(pcb,name,priority);
    //初始化内核栈布局
    struct ThreadStack*stack=(struct ThreadStack*)(pcb->kernelStack);
    stack->argument=arg;
    stack->ebp=stack->ebx=stack->edi=stack->esi=stack->retAddr=0;
    stack->eip=(uint32_t)KernelThread;
    stack->func=func;
    //加入队列
    ListPushBack(&ReadyList,&(pcb->tagS));
    ListPushBack(&ThreadList,&(pcb->tag));
    return pcb;
}

uint8_t AllocateTicks(uint8_t priority)
{
    return priority;
}

void InitKernalThread(){
    //初始化pid锁
    MutexInit(&pidLock);
    //
    mainThread=RunningThread();//当前正在运行得线程就是内核线程
    ThreadInit(mainThread,"kernel",1);
    //更改当前线程得状态
    mainThread->status=RUNNING;
    //把当前线程加入全局队列
    ListPushBack(&ThreadList,&(mainThread->tag));
}
void ExchangeThreadToReady(struct PCB*pcb){
    ListPushBack(&ReadyList,&pcb->tagS);//放到就绪队列末尾
    pcb->ticks=AllocateTicks(pcb->priority);//从新分配时间片
    pcb->status=READY;//状态变为就绪态
}
//处理延迟的任务
void ProcessDelayThread(){
    struct PCB*pcb;
    while((pcb=DelayMinHeap.buf[1].ptr,DelayMinHeap.curCnt&&systemTicks>=pcb->TargetDelayTime)){
        //将堆首弹出
        MinHeapPop(&DelayMinHeap);
        //将pcb从延迟队列移除
        ListRemove(&DelayList,&(pcb->tagS));
        //将pcb改为就绪状态
        ExchangeThreadToReady(pcb);
    }
}
//
//延迟最小堆的比较函数
bool DelayMinHeapCMP(MinHeapNode node0,MinHeapNode node1){
    struct PCB*pcb0=node0.ptr,*pcb1=node1.ptr;
    return pcb0->TargetDelayTime<pcb1->TargetDelayTime;
}
//
void init_thread()
{
    put_str("thread init start!\n");
    //
    systemTicks=0;
    //初始化所有的任务队列
    ListInit(&ReadyList);
    ListInit(&BlockList);
    ListInit(&DelayList);
    ListInit(&ThreadList);
    //初始化最小堆
    MinHeapInit(&DelayMinHeap,DelayMinHeapCMP,DelayMinHeapMaxCnt);
    //初始化内核主线程
    InitKernalThread();
    //
    put_str("thred init done!\n");
}

void schedule()
{
    ASSERT(interrupt_status()==0);
 
    struct PCB*cur=RunningThread();
    if(cur->status==RUNNING){//如果当前任务正在运行
        ExchangeThreadToReady(cur);
    }else if(cur->status==BLOCKED){//当前任务从运行态被阻塞
        ListPushBack(&BlockList,&(cur->tagS));//放入阻塞队列
    }else if(cur->status==DELAYED){//当前任务从运行态被延迟
        ListPushBack(&DelayList,&(cur->tagS));
        //放入延迟的最小堆
        MinHeapNode node;node.ptr=cur;
        MinHeapInsert(&DelayMinHeap,node);
    }
    //如果当前没有任务,唤醒闲置任务
    if(ListEmpty(&ReadyList)){
        ThreadUnBlock(idle_task);
    }
    //处理延迟的任务
    ProcessDelayThread();
    //切换任务
    struct ListNode*nxtTag=ListFront(&ReadyList);//从就绪队列取出第一个运行
    ListPopFront(&ReadyList);
    struct PCB*nxt=(struct PCB*)(((uint32_t)nxtTag)-MemberOffset(struct PCB,tagS));//获取pcb
    nxt->status=RUNNING;
    /*激活页表等*/
    ProcessActivate(nxt);
    //
    SwitchTo(cur,nxt);
}

void ThreadBlock()
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //////////////////////////////////
    struct PCB*pcb=RunningThread();//获取当前运行的线程
    ASSERT(pcb->status==RUNNING);//只有运行态的程序才可能被阻塞

    pcb->status=BLOCKED;
    schedule();
    //////////////////////////////////
    if(old)interrupt_enable();
}

void Thread_Yield()
{
    struct PCB*pcb=RunningThread();
    uint8_t old=interrupt_status();interrupt_disable();
    /////////////////////////////////////////
    //这里直接schedule是为了防止用户进程一直schedule，这样他的时间片会被重置
    pcb->status=READY;
    ListPushBack(&ReadyList,&(pcb->tagS));
    schedule();
    //////////////////////////////////////////
    if(old)interrupt_enable();
}

void ThreadUnBlock(struct PCB *pcb)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //////////////////////////////////
    
    pcb->status=READY;
    ListRemove(&BlockList,&(pcb->tagS));//从阻塞队列移除
    ListPushFront(&ReadyList,&(pcb->tagS));//加入就绪队列队首
    //////////////////////////////////
    if(old)interrupt_enable();
}

pid_t PidAllocate()
{
    static pid_t pid=0;
    MutexAcquire(&pidLock);
    pid_t ret=pid++;
    MutexRelease(&pidLock);
    return ret;
}

void SleepWithoutHang(uint32_t msecond)
{
    uint32_t end=systemTicks+msecond;
    while(end<=systemTicks){
        Thread_Yield();
    }
}