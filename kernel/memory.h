#ifndef MMZZ_MEMORY_H
#define MMZZ_MEMORY_H
#include "../lib/kernel/bitmap.h"
#include "../lib/kernel/list.h"
#include "global.h"
//////////////////////////////////////////
#define Memory_Block_Desc_CNT 7//描述符个数为7，即1028的内存
//////////////////////////////////////////
struct Vaddr_Pool{
    struct BitMap bp;
    uint32_t addr_start;
};//虚拟内存池

struct Memory_Block{
    struct ListNode tag;
};

struct Memory_Block_Desc{
    uint32_t blockSize;
    uint32_t CntPerPage;
    struct List freeMemoryBlock;    
};//

struct Arena{
    struct Memory_Block_Desc*desc;
    uint32_t cnt;
    bool largeMemory;//标识是否分配的是大内存
};

enum PoolOwner{
    KERNEL,
    USER
};

extern struct PhyAddr_Pool kernel_pool,user_pool;
void MemoryBlockDescInit(struct Memory_Block_Desc*desc_arr);
void mem_init();
void* VaddrGet(enum PoolOwner flag,uint32_t pageCnt);//从对应的虚拟地址空间分配出连续的虚拟空间
void* PTE_Addr(void*addr);//获取addr对应所在的页表的地址
void* PDE_Addr(void*addr);//获取addr对应所在的页目录表的地址
void* palloc(struct PhyAddr_Pool*pool);//从对应的物理内存池分配一页物理内存
void pfree(uint32_t phyAddr);//回收物理地址起始的页
void AddToPageTable(void*phyAddr,void*vAddr); //将虚拟地址和物理地址一一映射
void* malloc_page(enum PoolOwner flag,uint32_t pageCnt);//从对应的虚拟空间分配出相应数量的页
void free_page(enum PoolOwner flag,uint32_t vaddr,uint32_t cnt);//释放以vaddr起始的cnt个页（也会把物理页清空，相应的页表也会被重置）
void* mallocKernelPage(uint32_t pageCnt);//给内核申请内存
void* malloc_user_page(uint32_t pageCnt);//给用户分配出虚拟页
void* GetOnePage(enum PoolOwner flag,uint32_t vaddr);//将vaddr与一页物理页关联
uint32_t MapVaddrToPhyaddr(uint32_t vaddr);//把虚拟地址映射成物理地址
void* syscall_malloc(uint32_t size);
uint32_t syscall_free(void*vaddr);
void*GetOnePageWithoutOpBitmap(enum PoolOwner flag,uint32_t vaddr);
void FreeOnePhyPage(void* phyAddr);//释放一页物理内存(把对应的位图置0)
///////////////////////////////
#endif