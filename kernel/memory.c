#include "memory.h"
#include "thread.h"
/////////////////////////////////////////////////////////
#define TOTAL_MEMORY (*(uint32_t*)(0x800)) //总内存
#define BITMAP_BASE 0xc009a000 //内存图放置的位置
#define KERNEL_VADDR_START (0xc0100000) //内核虚拟地址开始的区间
#define PAGE_SIZE 4096//一页大小
#define KERNEL_FREEPAGE_PERCENT (2) //内核物理池占总空闲物理内存的百分比
#define PDE(addr) (((0xffc00000)&((uint32_t)(addr)))>>22) //获取当前地址所在页目录表的索引
#define PTE(addr) (((0x003ff000)&((uint32_t)(addr)))>>12) //获取当前地址所在页表的索引
/////////////////////////////////////////////////////////
struct PhyAddr_Pool{
    struct Mutex mutex;
    struct BitMap bp;
    uint32_t addr_start;
    uint32_t size;
};//物理内存池
/////////////////////////////////////////////////////////
struct PhyAddr_Pool kernel_pool,user_pool;
struct Vaddr_Pool kernel_vpool;
struct Memory_Block_Desc kernel_MBD[Memory_Block_Desc_CNT];//
void MemoryBlockDescInit(struct Memory_Block_Desc *desc_arr)
{
    uint32_t blocksize=16;
    for(int i=0;i<Memory_Block_Desc_CNT;++i,blocksize<<=1){
        desc_arr[i].blockSize=blocksize;
        desc_arr[i].CntPerPage=(PAGE_SIZE-sizeof(struct Arena))/blocksize;
        ListInit(&(desc_arr[i].freeMemoryBlock));
    }
}
struct Memory_Block* ArenaIdxBlock(struct Arena*arena,uint32_t idx){
    return (struct Memory_Block*)(
        ((uint32_t)arena)+sizeof(struct Arena)+idx*(arena->desc->blockSize)
    );
}
struct Arena* ArenaContainBlcok(struct Memory_Block*block){
    return (struct Arena*)(
      ((uint32_t)block)&0xfffff000
    );
}

void* syscall_malloc(uint32_t size){
    //判断当前进程类型
    enum PoolOwner flag=KERNEL;
    struct PCB*pcb=RunningThread();
    struct PhyAddr_Pool*pool=&kernel_pool;
    struct Memory_Block_Desc*desc=kernel_MBD;
    if(pcb->pageaddr)
    {
        flag=USER;
        pool=&user_pool;
        desc=pcb->user_MBD;
    }
    //
    struct Arena*arena=0;
    struct Memory_Block*block=0;
    MutexAcquire(&(pool->mutex));
    if(size>1024){
        uint32_t pageCnt=DIV_ROUND_UP(sizeof(struct Arena)+size,PAGE_SIZE);
        arena=malloc_page(flag,pageCnt);
        if(arena==0){
            MutexRelease(&(pool->mutex));
            return 0;
        }
        //对该区域内存清0
        memset(arena,0,pageCnt*PAGE_SIZE);
        arena->cnt=pageCnt;
        arena->largeMemory=1;
        arena->desc=0;
        MutexRelease(&(pool->mutex));
        return (void*)(arena+1);
    }else{
        for(int i=0;i<Memory_Block_Desc_CNT;++i){
            //找到第一个内存不小于要分配的内存的内存块
            if(desc[i].blockSize>=size){
                struct Memory_Block_Desc*target=&(desc[i]);
                //如果为空,分配一页
                if(ListEmpty(&(target->freeMemoryBlock))){
                    arena=malloc_page(flag,1);
                    if(arena==0){
                        MutexRelease(&(pool->mutex));
                        return 0;
                    }
                    memset(arena,0,PAGE_SIZE);
                    arena->largeMemory=0;
                    arena->cnt=target->CntPerPage;
                    arena->desc=target;
                    //拆分arena
                    uint8_t old=interrupt_status();interrupt_disable();//其实不用关中断也可以
                    for(int i=0;i<arena->cnt;++i){
                        ListPushBack(&(target->freeMemoryBlock),&(ArenaIdxBlock(arena,i)->tag));
                    }
                    if(old)interrupt_enable();                   
                }
                //分配内存
                struct ListNode*node=ListFront(&(target->freeMemoryBlock));
                ListPopFront(&(target->freeMemoryBlock));
                block=elem2entry(struct Memory_Block,tag,node);
                memset(block,0,target->blockSize);
                arena=ArenaContainBlcok(block);
                arena->cnt--;
                MutexRelease(&(pool->mutex));
                return block;
            }
        }
    }
    MutexRelease(&(pool->mutex));
    return 0;
}
uint32_t syscall_free(void*vaddr){
    uint32_t release=0;//释放的内存总容量
    //判断内存类型
    enum PoolOwner flag=KERNEL;
    struct PhyAddr_Pool*pool=&kernel_pool;
    struct PCB*pcb=RunningThread();
    if(pcb->pageaddr)
    {
        flag=USER;
        pool=&user_pool;
    }
    //
    struct Memory_Block*block=vaddr;
    struct Arena*arena=ArenaContainBlcok(block);
    MutexAcquire(&(pool->mutex));
    //如果是大块内存,直接释放
    if(arena->largeMemory){
        release=arena->cnt*PAGE_SIZE;
        free_page(flag,(uint32_t)vaddr,arena->cnt);
    }else{
        //将内存块放入闲置链表
        struct List*list=&(arena->desc->freeMemoryBlock);
        //
        arena->cnt++;
        ListPushBack(list,&(block->tag));
        //如果内存池的个数和起始个数一致，则释放内存
        if(arena->cnt==arena->desc->CntPerPage){
            //把空闲内存从desc移除
            for(int i=0;i<arena->cnt;++i){
                struct Memory_Block*block=ArenaIdxBlock(arena,i);
                ListRemove(list,&(block->tag));
            }
            free_page(flag,(uint32_t)(arena),1);
            release=PAGE_SIZE;
        }
    }
    //
    MutexRelease(&(pool->mutex));
    return release;
}
// 内核的虚拟内存图
void mem_init()
{
    put_str("memory pool init start!\n");
    put_str("total memory is:");
    put_num(TOTAL_MEMORY);
    put_char('\n');
    //在loader中已经初始化的页表的大小
    uint32_t table_size=PAGE_SIZE*256;//一共创建了256个页表(包括页目录表)
    uint32_t usedMem=table_size+0x100000;//低端1mb内存和页表内存
    uint32_t unUsedMem=TOTAL_MEMORY-usedMem;//剩下的空闲内存
    uint32_t freePage=unUsedMem/PAGE_SIZE;//可用的空闲页数
    //给内核和用户空间分配物理内存页
    uint32_t kernelFreePageCnt=freePage/KERNEL_FREEPAGE_PERCENT;
    uint32_t ueserFreePageCnt=freePage-kernelFreePageCnt;
    uint32_t kernelPhyMemStart=usedMem;
    uint32_t userPhyMemStart=kernelPhyMemStart+kernelFreePageCnt*PAGE_SIZE;
    kernel_pool.addr_start=kernelPhyMemStart;
    user_pool.addr_start=userPhyMemStart;
    kernel_pool.size=kernelFreePageCnt*PAGE_SIZE;
    user_pool.size=ueserFreePageCnt*PAGE_SIZE;
    //给内核和用户空间分配BitMap
    uint32_t kernel_bp_len=kernelFreePageCnt>>3;
    uint32_t user_bp_len=ueserFreePageCnt>>3;
    kernel_pool.bp.len=kernel_bp_len;
    user_pool.bp.len=user_bp_len;
    kernel_pool.bp.bits=(uint8_t*)BITMAP_BASE;
    user_pool.bp.bits=(uint8_t*)(BITMAP_BASE+kernel_bp_len);
    //初始化BitMap
    BitMapInit(&kernel_pool.bp);
    BitMapInit(&user_pool.bp);
    //初始化mutex
    MutexInit(&kernel_pool.mutex);
    MutexInit(&user_pool.mutex);
    //初始化内核虚拟内存图
    kernel_vpool.bp.len=kernel_bp_len;
    kernel_vpool.bp.bits=(uint8_t*)(BITMAP_BASE+kernel_bp_len+user_bp_len);
    kernel_vpool.addr_start=KERNEL_VADDR_START;
    BitMapInit(&kernel_vpool.bp);
    //初始化内核Memory_Block_Desc
    MemoryBlockDescInit(kernel_MBD);
    //
    put_str("memory pool init finished!\n");
}

void *VaddrGet(enum PoolOwner flag, uint32_t pageCnt)
{
   void*ret=0;
   if(flag==KERNEL){
    uint32_t idx=BitMapAllocate(&kernel_vpool.bp,pageCnt);
    if(idx!=-1){
        ret=(void*)(kernel_vpool.addr_start+idx*PAGE_SIZE);
        while(pageCnt--)BitMapSet(&kernel_vpool.bp,idx++);
    }
   }else{//用户内存池
    struct PCB*cur=RunningThread();
    uint32_t idx=BitMapAllocate(&cur->vpool.bp,pageCnt);
    if(idx!=-1){
        ret=(void*)(cur->vpool.addr_start+idx*PAGE_SIZE);
        while(pageCnt--)BitMapSet(&(cur->vpool.bp),idx++);
    }
   }
   return ret;
}

void *PTE_Addr(void *addr)
{
    uint32_t ret=(0xffc00000|((((uint32_t)addr)&0xffc00000)>>10)|(PTE(addr)<<2));
    return (void*)ret;
}

void *PDE_Addr(void *addr)
{
    uint32_t ret=(0xfffff000|((PDE(addr))<<2));
    return (void*)ret;
}

void *palloc(struct PhyAddr_Pool *pool)
{
    uint32_t idx=BitMapAllocate(&pool->bp,1);
    void*ret=0;
    if(idx!=-1){
        BitMapSet(&pool->bp,idx);
        ret=(void*)(pool->addr_start+idx*PAGE_SIZE);
    }
    return ret;
}

void pfree(uint32_t phyAddr)
{
    //判断物理地址属于哪个类型
    struct PhyAddr_Pool*pool=&user_pool;
    uint32_t idx;
    if(phyAddr<user_pool.addr_start){
        pool=&kernel_pool;
       
    }
    idx=(phyAddr-pool->addr_start)/PAGE_SIZE;
    //将位图表清0
    BitMapReset(&(pool->bp),idx);
}

//去掉虚拟地址在页表的存在位
void page_table_remove_pte(uint32_t vaddr){
    uint32_t*pte=PTE_Addr((void*)vaddr);
    *pte&=~PG_P_1;
    asm volatile("invlpg %0"::"m"(vaddr):"memory");//从缓存表去掉该项
}
//释放从vaddr虚拟地址开始的连续cnt个页
void VaddrRemove(enum PoolOwner flag,uint32_t vaddr,uint32_t cnt){
    struct Vaddr_Pool*pool;
    uint32_t idx;
    if(flag==KERNEL){
        pool=&kernel_vpool;
    }else if(flag==USER){
        pool=&(RunningThread()->vpool);
    }
    idx=(vaddr-pool->addr_start)/PAGE_SIZE;
    while(cnt--){
        BitMapReset(&(pool->bp),idx);
        ++idx;
    }
}

void free_page(enum PoolOwner flag,uint32_t vaddr,uint32_t cnt){
    uint32_t phyAddr=MapVaddrToPhyaddr(vaddr);//获取物理地址
    //判断内存类型(这里不可以通过runningthrad的类型判断，因为用户态也可以申请访问内核的空间，本质上释放的还是内核的空间)
    struct Vaddr_Pool*vpool=&kernel_vpool;
    struct PhyAddr_Pool*pool=&kernel_pool;
    if(phyAddr>=user_pool.addr_start){
        struct PCB*pcb=RunningThread();
        vpool=&(pcb->vpool);
        pool=&user_pool;
    }
    //////
    uint32_t vaddr_t=vaddr;
    uint32_t cnt_t=cnt;
    while(cnt_t--){
        //释放物理页
        phyAddr=MapVaddrToPhyaddr(vaddr_t);
        pfree(phyAddr);
        //从页目录项移除
        page_table_remove_pte(vaddr_t);
        vaddr_t+=PAGE_SIZE;
    }
    //从页表项移除
    VaddrRemove(flag,vaddr,cnt);
}
void AddToPageTable(void *phyAddr, void *vAddr)
{
    //先获取对应在页目录的地址,和在页表的地址
    uint32_t*pde=(uint32_t*)PDE_Addr(vAddr),*pte=(uint32_t*)PTE_Addr(vAddr);
    if((*pde)&1){//通过存在位判断是否已经建立了对应的页表
        ASSERT(!((*pte)&1));//
        *pte=((uint32_t)phyAddr)|PG_US_U|PG_RW_W|PG_P_1;
    }else{
        //如果不存在,那我们还得先建立一个页表
        int idx=BitMapAllocate(&kernel_pool.bp,1);
        if(~idx){
            void*pageAddr=palloc(&kernel_pool);
            if(pageAddr){      
                *pde=((uint32_t)pageAddr)|PG_US_U|PG_RW_W|PG_P_1;
                memset((void*)((uint32_t)pte & 0xfffff000),0,PAGE_SIZE);//对页表清0
                *pte=((uint32_t)phyAddr)|PG_US_U|PG_RW_W|PG_P_1;
            }
        }
    }
}

void* malloc_page(enum PoolOwner flag, uint32_t pageCnt)
{
    uint32_t vaddr=(uint32_t)VaddrGet(flag,pageCnt);
    if(vaddr==0)return 0;
    void*ret=(void*)vaddr;
    struct PhyAddr_Pool* pool=flag==KERNEL?&kernel_pool:&user_pool;
    //对每个虚拟地址进行映射
    while(pageCnt--){
        void*phyAddr=palloc(pool);
        if(!phyAddr){
            //如果物理内存不够了,需要回收之前分配出去的内存
            return 0;
        }
        AddToPageTable(phyAddr,(void*)vaddr);
        vaddr+=PAGE_SIZE;
    }
    return ret;
}

void *mallocKernelPage(uint32_t pageCnt)
{
    return malloc_page(KERNEL,pageCnt);
}

void *malloc_user_page(uint32_t pageCnt)
{
    MutexAcquire(&user_pool.mutex);
    void*vaddr=malloc_page(USER,pageCnt);
    memset(vaddr,0,PAGE_SIZE*pageCnt);
    MutexRelease(&user_pool.mutex);
    return vaddr;
}

void* GetOnePage(enum PoolOwner flag, uint32_t vaddr)
{
    struct PhyAddr_Pool*pool=flag==KERNEL?&kernel_pool:&user_pool;
    MutexAcquire(&pool->mutex);
    struct PCB*cur=RunningThread();
    uint32_t idx=-1;
    //如果是用户进程
    if(cur->pageaddr&&flag==USER){
        idx=(vaddr-cur->vpool.addr_start)/PAGE_SIZE;
    }else if(flag==KERNEL){
        idx=(vaddr-kernel_vpool.addr_start)/PAGE_SIZE;
    }else{
        PANIC_MACRO("HERE!");
    }
    BitMapSet(flag==KERNEL?&kernel_vpool.bp:&(cur->vpool.bp),idx);
    void*phyaddr=palloc(pool);
    if(phyaddr==0){
        //还需回收虚拟内存
        MutexRelease(&pool->mutex);
        return 0;
    }
    AddToPageTable(phyaddr,(void*)vaddr);
    MutexRelease(&pool->mutex);
    return (void*)vaddr;
}
//与上面紧挨得那个函数唯一的区别就是他不会操作位图
void*GetOnePageWithoutOpBitmap(enum PoolOwner flag,uint32_t vaddr){
    struct PhyAddr_Pool*pool=flag==KERNEL?&kernel_pool:&user_pool;
    MutexAcquire(&pool->mutex);
    void*phyaddr=palloc(pool);
    if(phyaddr==0)
    {
        MutexRelease(&pool->mutex);
        return 0;
    }

    AddToPageTable(phyaddr,(void*)vaddr);
    MutexRelease(&pool->mutex);
    return (void*)vaddr;
}

void FreeOnePhyPage(void *phyAddr)
{
    struct PhyAddr_Pool*pool=&kernel_pool;
    if(((uint32_t)phyAddr)>=user_pool.addr_start){
        pool=&user_pool;
    }
    uint32_t idx=((uint32_t)phyAddr-pool->addr_start)/PAGE_SIZE;
    BitMapReset(&(pool->bp),idx);
}

uint32_t MapVaddrToPhyaddr(uint32_t vaddr)
{
    uint32_t* pte=(uint32_t*)PTE_Addr((void*)vaddr);//获取在pte表中的虚拟地址
    return (uint32_t)((*pte &0xfffff000)+(vaddr&0xfff));//实际物理地址(实际上这里前一个&可以去掉，因为保证页起始地址都是4k的倍数)
}

