#include"wait_exit.h"
#include"../fs/file.h"
//释放进程所有的资源
void ReleaseProcessResource(struct PCB*pcb){
    ////////////////回收内存资源
    uint32_t*pgVaddr=(uint32_t*)(pcb->pageaddr);
    //用户进程占的虚拟内存最多到第768项
    for(int i=0;i<768;++i){
        uint32_t pde=pgVaddr[i];
        //如果pde存在
        if(pde&1){
            uint32_t* pteVaddr=PTE_Addr((void*)(i*0x400000));
            //一个页框最多有1024个页
            for(int j=0;j<1024;++j){
                uint32_t pte=pteVaddr[j];
                //如果pte存在
                if(pte&1){
                    uint32_t phyAddr=pte&0xfffff000;
                    FreeOnePhyPage((void*)phyAddr);
                }
            }
            uint32_t phyAddr=pde&0xfffff000;
            FreeOnePhyPage((void*)phyAddr);
        }
    }
    ////////////////////////回收虚拟地址位图占的内核内存
    uint32_t bpPageCnt=pcb->vpool.bp.len/PAGE_SIZE;
    free_page(KERNEL,(uint32_t)(pcb->vpool.bp.bits),bpPageCnt);
    ///////////////////////关闭文件
    extern int32_t syscall_close(int32_t fd_l);
    for(int i=STD_FD_CNT;i<MAX_FILE_CNT_OPEN_PROCESS;++i){
        if(pcb->fd[i]!=-1){
            syscall_close(pcb->fd[i]);
        }
    }
}
//收养所有父进程为pid的进程
void InitAdopt(pid_t pid){
    struct ListNode*cur=ThreadList.head.nxt;
    struct ListNode*end=&(ThreadList.tail);
    while(cur!=end){
        struct PCB*child=(struct PCB*)elem2entry(struct PCB,tag,cur);
        if(child->pa_pid==pid){
            child->pa_pid=1;//1就是init进程的pid,系统已经保证
        }
        cur=cur->nxt;
    }
}
struct PCB* FindTargetPCB(pid_t pid){
    struct ListNode*cur=ThreadList.head.nxt;
    struct ListNode*end=&(ThreadList.tail);
    while(cur!=end){
        struct PCB*fa=(struct PCB*)elem2entry(struct PCB,tag,cur);
        if(fa->pid==pid){
            return fa;
        }
        cur=cur->nxt;
    }
    return 0;
}
pid_t syscall_wait(int32_t *status)
{
    struct PCB*pcb=RunningThread();
    while(1){
        bool flag=0;
        //寻找是否有子进程状态变为悬浮态
        struct ListNode*cur=ThreadList.head.nxt;
        struct ListNode*end=&(ThreadList.tail);
        while(cur!=end){
            struct PCB*child=(struct PCB*)elem2entry(struct PCB,tag,cur);
            if(child->pa_pid==pcb->pid){
                flag=1;
                if(child->status==HANGING){
                    pid_t pid=child->pid;
                    *status=child->exit_status;
                    //释放掉子进程占的最后空间
                    ThreadExit(child,0);
                    return pid;
                }
            }
            cur=cur->nxt;
        }
        //都没有子进程,只能返回-1
        if(!flag)return -1;
        //反之阻塞自己
        ThreadBlock(WAITING);
    }
    return -1;
}

void syscall_exit(int32_t code)
{
    struct PCB*pcb=RunningThread();
    pcb->exit_status=code;
    ASSERT(pcb->pa_pid!=-1);//除了系统刚开始初始化的几个线程，不可能有其他进程的父进程为-1
    //将进程所有的子进程过继给init进程收养
    InitAdopt(pcb->pid);
    //回收进程的资源
    ReleaseProcessResource(pcb);
    //找到父进程,看它是否是waiting状态
    struct PCB*fa=FindTargetPCB(pcb->pa_pid);
    ASSERT(fa);//fa按道理是不可能为0的
    if(fa->status==WAITING){
        ThreadUnBlock(fa);
    }
    //挂起,等待最后父进程的回收
    ThreadBlock(HANGING);
}
