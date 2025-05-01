#include "process.h"
void* ProcessCreatePage(){
    uint32_t pde=(uint32_t)mallocKernelPage(1);
    ASSERT(pde);
    memcpy((void*)(pde+0x300*4),(void*)(0xfffff000+0x300*4),1024);//为了使得用户进程可以访问到内核,把内核页表项copy过去
    uint32_t pagePhyAddr=MapVaddrToPhyaddr(pde);
    ((uint32_t*)pde)[1023]=pagePhyAddr|PG_US_U|PG_RW_W|PG_P_1;//页目录最后一项指向页目录本身
    return (void*)pde;
}
void ProcessCreateVaddrBitmap(struct PCB*pcb){
    pcb->vpool.addr_start=USER_VADDR_START;
    uint32_t bpPageCnt=DIV_ROUND_UP((0xc0000000-USER_VADDR_START)/PAGE_SIZE/8,PAGE_SIZE);
    void*mem=mallocKernelPage(bpPageCnt);
    ASSERT(mem);
    pcb->vpool.bp.bits=mem;
    pcb->vpool.bp.len=(0xc0000000-USER_VADDR_START)/PAGE_SIZE/8;
    BitMapInit(&(pcb->vpool.bp));
}
void PageDirActivate(struct PCB *pcb)
{
    uint32_t phyAddr=0x100000;//内核线程的页表地址
    if(pcb->pageaddr){
        phyAddr=MapVaddrToPhyaddr(pcb->pageaddr);
    }
    asm volatile("movl %0,%%cr3"::"r"(phyAddr):"memory");
}

void ProcessActivate(struct PCB *pcb)
{
    ASSERT(pcb);
    PageDirActivate(pcb);
    if(pcb->pageaddr){
        TssUpdateEsp(pcb);
    }
}

void ProcessStart(void *file)
{
    //
    void*fun=file;
    struct PCB*pcb=RunningThread();
    pcb->kernelStack+=sizeof(struct ThreadStack);//跳过线程栈
    struct InterruptStack*itr_stk=(struct InterruptStack*)(pcb->kernelStack);
    memset(itr_stk,0,sizeof(struct InterruptStack));
    itr_stk->ds=itr_stk->es=itr_stk->fs=SELECTOR_U_DATA;
    itr_stk->cs=SELECTOR_U_CODE;
    itr_stk->eip=(uint32_t)fun;
    itr_stk->eflags=(EFLAGS_IOPL_0|EFLAGS_MBS|EFLAGS_IF_1);
    itr_stk->esp=(uint32_t)(GetOnePage(USER,USER_STACK3_VADDR))+PAGE_SIZE;
    itr_stk->ss=SELECTOR_U_DATA;
    asm volatile("movl %0,%%esp;jmp itr_exit"::"g"(itr_stk):"memory");
}

void ProcessExe(void *fileName, char *name)
{
    struct PCB*pcb=mallocKernelPage(1);//维护pcb信息
    ASSERT(pcb!=0);//
    ThreadInit(pcb,name,USER_PROCESS_PRIORITY);
    ProcessCreateVaddrBitmap(pcb);
    //初始化内核栈布局
    struct ThreadStack*stack=(struct ThreadStack*)(pcb->kernelStack);
    stack->argument=fileName;
    stack->ebp=stack->ebx=stack->edi=stack->esi=stack->retAddr=0;
    stack->eip=(uint32_t)KernelThread;
    stack->func=ProcessStart;
    //
    pcb->pageaddr=(uint32_t)ProcessCreatePage();
    MemoryBlockDescInit(pcb->user_MBD);
    //添加到就绪队列和全局队列
    uint8_t old=interrupt_status();
    interrupt_disable();
    ListPushBack(&ReadyList,&(pcb->tagS));
    ListPushBack(&ThreadList,&(pcb->tag));
    if(old)interrupt_enable();
}
