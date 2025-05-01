#include"fork.h"
#include"../fs/file.h"
extern void itr_exit();
//拷贝父进程的pcb、虚拟内存图和0级栈(目前没考虑失败回溯处理)
bool CopyPCB_VaddrBM_Stack0(struct PCB*parent,struct PCB*child){
    memcpy(child,parent,PAGE_SIZE);
    //修改pcb
    child->pid=PidAllocate();
    child->pa_pid=parent->pid;
    child->status=READY;
    child->ticks=AllocateTicks(child->priority);
    child->totalTicks=0;
    MemoryBlockDescInit(child->user_MBD);
    //赋值vaddr_bitmap
    uint32_t bpCnt=DIV_ROUND_UP((0xc0000000-USER_VADDR_START)/PAGE_SIZE/8,PAGE_SIZE);
    void*bpAddr=mallocKernelPage(bpCnt);
    memcpy(bpAddr,parent->vpool.bp.bits,bpCnt*PAGE_SIZE);
    child->vpool.bp.bits=bpAddr;
    return 1;
}

//复制父进程的3级用户栈(即copy用户虚拟内存)
bool CopyStack3(struct PCB*parent,struct PCB*child,void*buf){
    int32_t len=parent->vpool.bp.len;
    uint8_t*bp=parent->vpool.bp.bits;
    uint32_t vaddr_base=parent->vpool.addr_start;
    for(int idx0=0,idx1;idx0<len;++idx0,++bp){
        uint8_t v=*bp;
        if(!v)continue;
        for(idx1=0;idx1<8;++idx1){
            if(v>>idx1&1){
                uint32_t vaddr=vaddr_base+((idx0<<3)+idx1)*PAGE_SIZE;
                memcpy(buf,(void*)vaddr,PAGE_SIZE);
                PageDirActivate(child);
                GetOnePageWithoutOpBitmap(USER,vaddr);
                memcpy((void*)vaddr,buf,PAGE_SIZE);
                PageDirActivate(parent);
            }
        }
    }
    return 1;
}
//构建子进程的返回栈
bool BuildChildThreadStack(struct PCB*child){
    struct InterruptStack*stack=(struct InterruptStack*)((uint32_t)child+PAGE_SIZE-sizeof(struct InterruptStack));
    stack->eax=0;//返回值为0，因为子进程fork返回值为0
    /*根据switchTo的寄存器布局
    ebp=stack-20
    ebx=stack-16
    edi=stack-12
    esi=stack-8
    ret=stack-4
    */
    uint32_t*ret=((uint32_t*)stack)-1;
    *ret=(uint32_t)itr_exit;
    child->kernelStack=(uint32_t)(((uint32_t*)stack)-5);
    return 1;
}
//共享文件
bool UpdateInode(struct PCB*child){
    for(int i=STD_FD_CNT;i<MAX_FILE_CNT_OPEN_PROCESS;++i){
        int fd=child->fd[i];
        if(fd!=-1){
            GlobalFileTable[fd].inode->open_cnt++;
        }
    }
    return 1;
}

bool CopyProcess(struct PCB*parent,struct PCB*child){
    void*buf=mallocKernelPage(1);
    ASSERT(buf);
    int rb=0;
    //
    if(!CopyPCB_VaddrBM_Stack0(parent,child)){
        goto CopyProcessRB;
    }
    //
    child->pageaddr=(uint32_t)ProcessCreatePage();//创建页表
    if(child->pageaddr==0){
        goto CopyProcessRB;
    }
    //
    if(!CopyStack3(parent,child,buf)){
        rb=1;
        goto CopyProcessRB;
    }
    //
    if(!BuildChildThreadStack(child)){
        rb=1;
        goto CopyProcessRB;
    }
    //
    if(!UpdateInode(child)){
        rb=1;
        goto CopyProcessRB;
    }
    //
    free_page(KERNEL,(uint32_t)buf,1);
    return 1;
    CopyProcessRB:
        switch(rb){
            case 1:     
            free_page(KERNEL,(uint32_t)child->pageaddr,1);
            case 0:
            free_page(KERNEL,(uint32_t)buf,1);
        }
        return 0;
}

int32_t syscall_fork()
{
    struct PCB*parent=RunningThread();
    struct PCB*child=mallocKernelPage(1);
    if(child==0)return -1;
    if(!CopyProcess(parent,child)){
        free_page(KERNEL,(uint32_t)child,1);
        return -1;
    }
    ListPushBack(&(ReadyList),&(child->tagS));
    ListPushBack(&(ThreadList),&(child->tag));
    return child->pid;
}

int32_t fork()
{
    return syscall(SYSCALL_FORK);
}
