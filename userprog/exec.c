#include"exec.h"
#include"process.h"
#include"../lib/math.h"
#include"../fs/fs.h"
#include"../fs/file.h"
#include"../kernel/memory.h"
///////////////////////////////////////////////////////////
extern int32_t syscall_seek(int32_t fd,int32_t offset,uint32_t flag);
extern int32_t syscall_read(int32_t fd_l,void*buf,uint32_t bytes);
extern int32_t syscall_open(const char*file,uint32_t flag);
extern int32_t syscall_close(int32_t fd_l);
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
bool SegmentLoad(int32_t fd,uint32_t offset,uint32_t filesize,uint32_t vaddr){
    uint32_t first_page=vaddr&0xfffff000;
    uint32_t size_in_first_page=PAGE_SIZE-(vaddr&0xfff);
    int pageneed=1;
    if(filesize>size_in_first_page){
        int left=filesize-size_in_first_page;
        pageneed=DIV_ROUND_UP(left,PAGE_SIZE)+1;
    }
    //分配内存
    for(int i=0;i<pageneed;++i,first_page+=PAGE_SIZE){
        uint32_t*pde=PDE_Addr((void*)first_page);
        uint32_t*pte=PTE_Addr((void*)first_page);
        if(!((*pde)&1)||!((*pte)&1)){
            if(!GetOnePage(USER,first_page))return 0;
        }
    }
    //
    bool flag=syscall_seek(fd,offset,SEEK_SET);
    int32_t size=syscall_read(fd,(void*)vaddr,filesize);
    return flag&&(size==filesize);
}

uint32_t load(const char*pathname){
    uint32_t ret=-1;
    struct Elf32_Ehdr elf_header;
    struct Elf32_Phdr prog_header;
    //
    int32_t fd=syscall_open(pathname,O_RDONLY);
    if(fd==-1)return -1;
    //读取elf头
    if(syscall_read(fd,&elf_header,sizeof(elf_header))!=sizeof(elf_header)){
        goto LoadFail;
    }
    if(memcmp(elf_header.e_ident,"\177ELF\1\1\1",7) || elf_header.e_machine != 3 || elf_header.e_type != 2 || \
       elf_header.e_version != 1 || elf_header.e_phnum > 1024 || \
       elf_header.e_phentsize != sizeof(struct Elf32_Phdr))
    {
        printk("load: memcmp fail\n");
        goto LoadFail;
    }
    //
    Elf32_Off  prog_header_offset = elf_header.e_phoff;
    Elf32_Half prog_header_size   = elf_header.e_phentsize;
    
    //遍历程序头 根据e_phnum来遍历 具体偏移位置由程序头表里面的offset决定 offset是给出下个程序头距离当前的头的巨鹿
    uint32_t prog_idx = 0;
    while(prog_idx < elf_header.e_phnum)
    {
        memset(&prog_header,0,prog_header_size);
        syscall_seek(fd,prog_header_offset,SEEK_SET);
        if(syscall_read(fd,&prog_header,prog_header_size) != prog_header_size)goto LoadFail;
        //可用于加载的有效段
        if(PT_LOAD == prog_header.p_type)
        {   
            //载入到内存中
            if(!SegmentLoad(fd,prog_header.p_offset,prog_header.p_filesz,prog_header.p_vaddr))
                goto LoadFail;
        }
        
        prog_header_offset += elf_header.e_phentsize;
        ++prog_idx;
    }
    
    ret = elf_header.e_entry;  //入口点地址
    //
    LoadFail:
        syscall_close(fd);
        return ret;
}

int32_t syscall_exec(const char*path,int32_t argc,char*argv[]){
    uint32_t entryPoint=load(path);
    if(entryPoint==-1)return -1;
    //把当前的进程修改掉
    struct PCB*pcb=RunningThread();
    //修改进程名
    {
        const char*ptr=path+strlen(path);
        while(*ptr!='/')--ptr;
        ++ptr;
        int copySize=min(strlen(ptr),THREAD_NAME_MAX_LEN);
        memcpy(pcb->name,ptr,copySize);
        pcb->name[copySize]=0;
    }
    //构建内核栈
    extern void itr_exit();
    struct InterruptStack*stack=(struct InterruptStack*)((uint32_t)pcb+PAGE_SIZE-sizeof(struct InterruptStack));
    stack->ebx=argc;
    stack->ecx=(uint32_t)argv;
    stack->eip=(uint32_t)entryPoint;
    stack->esp=(uint32_t)0xc0000000;//用户进程的栈地址为用户进程虚拟地址最高地址
    //
    asm volatile("movl %0,%%esp;jmp itr_exit"::"g"(stack):"memory");
    return 0;
}