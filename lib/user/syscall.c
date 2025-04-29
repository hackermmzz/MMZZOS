#include "syscall.h"
#include "../../fs/file.h"
#include "../../fs/dir.h"
#include"../../fs/super_block.h"
#include "../../fs/inode.h"
//////////////////////////////////////////////////
#define SYSCALL_CNT 128
//////////////////////////////////////////////////
uint32_t syscall_table[SYSCALL_CNT+1];//
uint8_t syscall_ArgInfo[SYSCALL_CNT+1];//系统调用参数配置表
///////////////////////////////////////////////////系统调用函数
void MakeSyscallTable(uint32_t num,uint32_t funcAddr,uint8_t ArgCnt){
    syscall_ArgInfo[num]=ArgCnt;
    syscall_table[num]=funcAddr;
}
pid_t syscall_GetProcessPid(){
    struct PCB*pcb=RunningThread();
    return pcb->pid;
}
uint32_t syscall_write(uint32_t fd,const byte*buf,uint32_t size){
    //如果fd是标准流
    if(fd==STDOUT_FD){
         printk("%s",buf);
         return size;
    }
    else if(fd>=STD_FD_CNT){
        fd=LocalFdToGlobalFd(fd);
        if(fd==-1)return 0;
        return file_write(fd,buf,size);
    }
    return 0;
}

uint32_t syscall_sleep(uint32_t msecond){
    struct PCB*pcb=RunningThread();
    pcb->TargetDelayTime=msecond+systemTicks;//一次滴答为1ms
    pcb->status=DELAYED;
    schedule();
    return msecond;
}
///////////////////////////////////////////////////
int32_t syscall_open(const char*file,uint32_t flag){
    if(file[strlen(file)-1]=='/')return -1;//最后不能是/结尾
    //先检查文件是否存在
    char name[FILENAME_MAX_LEN+1];
    struct DirEntry entry;
    struct Dir*dir=SearchFile(file,name,&entry,0);
    if(dir==0&&entry.filetype==FT_UNKOWN){
        //如果连最后一层都没解析到
        return -1;
    }
    bool found=entry.filetype==FT_FILE;//文件是否存在
    if(!found&&!(flag&O_CREATE)){
        //如果文件不存在并且也没有创建的flag
        if(dir)DirClose(dir);
        return -1;
    }
    if(found&&(flag&O_CREATE)){
        //如果已经存在了还去创建
        if(dir)DirClose(dir);
        return -1;
    }
    int32_t fd=-1;
    switch (flag&O_CREATE)
    {
    case O_CREATE:
        fd=file_create(dir,name,flag);
        break;
    default:
        fd=file_open(entry.index,flag);
    }
    return fd;
}

int32_t syscall_close(int32_t fd_l){
    if(fd_l<STD_FD_CNT)return 0;//标准输入输出流等都不允许关闭
    struct PCB*pcb=RunningThread();
    int fd_g=LocalFdToGlobalFd(fd_l);
    pcb->fd[fd_l]=-1;//关闭局部描述符
    if(fd_g==-1)return 0;//关闭失败
    //
    struct FILE*file=&(GlobalFileTable[fd_g]);
    if(file->inode){
        InodeClose(file->inode);
        file->inode=0;//回收全局描述符
        return 1;//关闭成功
    }
    return 0;
}
int32_t syscall_read(int32_t fd_l,void*buf,uint32_t bytes){
    if(fd_l==STDIN_FD){

    }else if(fd_l>=STD_FD_CNT){
        int fd=LocalFdToGlobalFd(fd_l);
        if(fd==-1)return 0;
        return file_read(fd,buf,bytes);
    }
    return 0;
}
///////////////////////////////////////////////////

int32_t syscall_seek(int32_t fd,int32_t offset,uint32_t flag){
    fd=LocalFdToGlobalFd(fd);
    if(fd==-1)return 0;
    int32_t fileSize=GlobalFileTable[fd].inode->size;
    int32_t pos=GlobalFileTable[fd].fd_pos;
    int32_t newpos;
    if(flag==SEEK_SET)newpos=offset;
    else if(flag==SEEK_CUR)newpos=pos+offset;
    else if(flag==SEEK_END)newpos=fileSize+offset;
    if(newpos<0||newpos>=fileSize){
        return 0;
    }
    GlobalFileTable[fd].fd_pos=newpos;
    return 1;
}

int32_t syscall_unlink(const char*file){
    //先检查文件是否存在
    char name[FILENAME_MAX_LEN+1];
    int32_t parent;
    struct DirEntry entry;
    struct Dir*dir=SearchFile(file,name,&entry,&parent);
    if(dir)DirClose(dir);//关闭目录
    if(entry.filetype!=FT_FILE)return 0;//如果不存在或者不是普通文件直接返回
    //先检查文件是否已经被打开
    if(FindOpenInode(CurPartition,entry.index))return 0;
    //回收掉inode
    struct Inode*inode=InodeOpen(CurPartition,entry.index);
    ASSERT(inode);
    uint32_t addr[140];
    ASSERT(InodeRecycle(CurPartition,inode,addr));
    InodeClose(inode);
    //删除在父目录的记录
    ASSERT(DeleteDirectoryDirEntry(CurPartition,parent,name,addr));//删不掉属于系统级别的硬伤，直接卡死
    return 1;
}
int32_t syscall_mkdir(const char*path){
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    struct Dir*dir=SearchFile(path,name,&entry,0);
    if(!dir)return 0;
    //创建3个目录项,分别是path下的. .. 和path父目录下的path
    int idx=-1;
    idx=InodeBitmapAllocate(CurPartition);
    if(idx==-1) goto MkDirRollBack;
    //path
    int32_t faIdx=dir->inode->index;
    entry.index=idx;
    memcpy(entry.filename,name,sizeof(name));
    entry.filetype=FT_DIR;
    if(!CreateDirEntry(CurPartition,dir,&entry))goto MkDirRollBack;
    dir->inode->size++;
    InodeWrite(CurPartition,dir->inode);//同步到磁盘
    DirClose(dir);//关闭目录
    dir=DirOpen(CurPartition,idx);//打开新建的目录
    dir->inode->size=2;
    //.
    entry.index=dir->inode->index;
    memcpy(entry.filename,".",2);
    entry.filetype=FT_DIR;
    if(!CreateDirEntry(CurPartition,dir,&entry))goto MkDirRollBack;
    //..
    entry.index=faIdx;//..指向父亲节点
    memcpy(entry.filename,"..",3);
    entry.filetype=FT_DIR;
    if(!CreateDirEntry(CurPartition,dir,&entry))goto MkDirRollBack;
    //同步到磁盘
    InodeWrite(CurPartition,dir->inode);
    BitMapUpdate(CurPartition,idx,BITMAP_FOR_INODE);
    DirClose(dir);
    return 1;
    //
    MkDirRollBack:
        BitMapReset(&(CurPartition->inode_bitmap),idx);
        DirClose(dir);
        return 0;
}
//
struct Dir*syscall_opendir(const char*path){
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    struct Dir*dir=SearchFile(path,name,&entry,0);
    if(dir)DirClose(dir);
    if(entry.filetype!=FT_DIR)return 0;//只要最后查找的目录项不是目录，返回0
    return DirOpen(CurPartition,entry.index);
}
//
int32_t syscall_closedir(struct Dir*dir){
    if(dir)
    {
        DirClose(dir);
        return 1;
    }
    return 0;
}
//
int32_t syscall_rewinddir(struct Dir*dir){
    if(dir){
        dir->dir_pos=0;
        return 1;
    }
    return 0;
}
//
struct DirEntry* syscall_readdir(struct Dir*dir){
    if(!dir||dir->dir_pos>=dir->inode->size)return 0;
    uint32_t addr[140];
    memcpy(addr,dir->inode->i_sectors,48);
    if(dir->inode->i_sectors[12])
    ide_read(CurPartition->my_disk,(void*)(((uint32_t)addr)+48),dir->inode->i_sectors[12],1);
    int32_t tarpos=dir->dir_pos;
    int32_t idx0=0;
    int32_t curpos=0;
    int32_t cnt=SECTOR_SIZE/sizeof(struct DirEntry);
    while(curpos!=tarpos){
        if(addr[idx0]==0){
            ++idx0;
            continue;
        }
        //
        ide_read(CurPartition->my_disk,dir->dir_buf,addr[idx0],1);
        struct DirEntry*entrys=(struct DirEntry*)dir->dir_buf;
        for(int i=0;i<cnt;++i){
            struct DirEntry*entry=&entrys[i];
            if(entry->filetype!=FT_UNKOWN){
                ++curpos;
                if(curpos==tarpos){
                    ++(dir->dir_pos);
                    return entry;
                }
            }
        }
    }
    return 0;
}
//
int32_t syscall_rmdir(const char*path){
    //先判断目录是否存在
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent);
    if(dir)DirClose(dir);//关闭这个目录
    if(parent==-1||entry.filetype!=FT_DIR)return 0;//如果该目录不存在
    //判断要删除的目录是否被打开了
    if(FindOpenInode(CurPartition,entry.index))return 0;//如果被打开了，那么是不能删除的
    //判断要删除的那个目录是否为空
    struct Dir*target=DirOpen(CurPartition,entry.index);
    ASSERT(target);
    if(target->inode->size!=2){
        DirClose(target);
        return 0;
    }
    //删除在父目录的记录
    uint32_t addr[140];
    ASSERT(DeleteDirectoryDirEntry(CurPartition,target->inode->index,name,addr));
    //回收掉inode
    ASSERT(InodeRecycle(CurPartition,target->inode,addr));
    DirClose(target);
    return 1;
}
//
int32_t syscall_getcwd(char*buf){
    struct PCB*pcb=RunningThread();
    struct Dir* dir=pcb->workDir;
    ASSERT(dir);
    //
    
}
//
int32_t syscall_chdir(const char*path){
    //先判断目录是否存在
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent);
    if(dir)DirClose(dir);//关闭这个目录
    if(parent==-1||entry.filetype!=FT_DIR)return 0;//如果该目录不存在
    //关闭原目录
    struct PCB*pcb=RunningThread();
    DirClose(pcb->workDir);
    ASSERT(pcb->workDir);
    //打开新目录
    pcb->workDir=DirOpen(CurPartition,entry.index);
    return 1;
}
//
void syscall_init()
{
    put_str("syscall table init start!\n");
    ///////////////////////////////////////////////////
    memset(syscall_ArgInfo,-1,sizeof(syscall_ArgInfo));
    extern void* syscall_malloc(uint32_t);
    extern uint32_t syscall_free(void*);
    ///////////////////////////////////////////////////
    MakeSyscallTable(SYSCALL_GETPID,(uint32_t)(void*)syscall_GetProcessPid,0);
    MakeSyscallTable(SYSCALL_WRITE,(uint32_t)(void*)syscall_write,3);
    MakeSyscallTable(SYSCALL_MALLOC,(uint32_t)(void*)syscall_malloc,1);
    MakeSyscallTable(SYSCALL_FREE,(uint32_t)(void*)syscall_free,1);
    MakeSyscallTable(SYSCALL_SLEEP,(uint32_t)(void*)syscall_sleep,1);
    MakeSyscallTable(SYSCALL_OPEN,(uint32_t)(void*)syscall_open,2);
    MakeSyscallTable(SYSCALL_CLOSE,(uint32_t)(void*)syscall_close,1);
    MakeSyscallTable(SYSCALL_READ,(uint32_t)(void*)syscall_read,3);
    MakeSyscallTable(SYSCALL_SEEK,(uint32_t)(void*)syscall_seek,3);
    MakeSyscallTable(SYSCALL_UNLINK,(uint32_t)(void*)syscall_unlink,1);
    MakeSyscallTable(SYSCALL_MKDIR,(uint32_t)(void*)syscall_mkdir,1);
    MakeSyscallTable(SYSCALL_OPENDIR,(uint32_t)(void*)syscall_opendir,1);
    MakeSyscallTable(SYSCALL_CLOSEDIR,(uint32_t)(void*)syscall_closedir,1);
    MakeSyscallTable(SYSCALL_CLOSEDIR,(uint32_t)(void*)syscall_rewinddir,1);
    MakeSyscallTable(SYSCALL_READDIR,(uint32_t)(void*)syscall_readdir,1);
    MakeSyscallTable(SYSCALL_DIRREMOVE,(uint32_t)(void*)syscall_rmdir,1);
    MakeSyscallTable(SYSCALL_GETCWD,(uint32_t)(void*)syscall_getcwd,1);
    MakeSyscallTable(SYSCALL_CHDIR,(uint32_t)(void*)syscall_chdir,1);
    ////////////////////////////////////////////////////
    put_str("syscall table init done!\n");
}
int syscall(int number, ...)
{
    ASSERT(number<=SYSCALL_CNT&&number>=0);//
    va_list args;
    va_start(args, number);
    ///////////////////////////////////
    uint8_t argcnt=syscall_ArgInfo[number];
    ASSERT(argcnt!=(uint8_t)-1);//
    if(argcnt==0)return Syscall0(number);
    else if(argcnt==1)
    {
        int v0=va_arg(args,int);
        return Syscall1(number,v0);
    }
    else if(argcnt==2)
    {
        int v0=va_arg(args,int);
        int v1=va_arg(args,int);
        return Syscall2(number,v0,v1);
    }
    else if(argcnt==3)
    {
        int v0=va_arg(args,int);
        int v1=va_arg(args,int);
        int v2=va_arg(args,int);
        return Syscall3(number,v0,v1,v2);
    }
    ///////////////////////////////////
    va_end(args);
    return -1;
}

int Syscall0(int num)
{
    int ret;
    asm volatile("int $0x80":"=a"(ret):"a"(num));
    return ret;
}

int Syscall1(int num,int a0)
{
    int ret;
    asm volatile("int $0x80":"=a"(ret):"a"(num),"b"(a0));
    return ret;
}

int Syscall2(int num,int a0, int a1)
{
    int ret;
    asm volatile("int $0x80":"=a"(ret):"a"(num),"b"(a0),"c"(a1));
    return ret;
}

int Syscall3(int num,int a0, int a1,int a2)
{
    int ret;
    asm volatile("int $0x80":"=a"(ret):"a"(num),"b"(a0),"c"(a1),"d"(a2));
    return ret;
}

uint32_t getpid()
{
    return syscall(SYSCALL_GETPID);
}

uint32_t write(uint32_t fd, const byte *buf, uint32_t size)
{
    return syscall(SYSCALL_WRITE,fd,buf,size);
}

uint32_t read(uint32_t fd, void*buf, uint32_t size)
{
    return syscall_read(fd,buf,size);
}

void *malloc(uint32_t size)
{
    return (void*)syscall(SYSCALL_MALLOC,size);
}

void free(void *addr)
{
    syscall(SYSCALL_FREE,addr);
}

void sleep(uint32_t msecond)
{
    syscall(SYSCALL_SLEEP,msecond);
}

int32_t open(const char *filename, uint8_t flag)
{
    return syscall(SYSCALL_OPEN,filename,(uint32_t)flag);
}

int32_t close(int32_t fd)
{
    return syscall(SYSCALL_CLOSE,fd);
}

int32_t seek(int32_t fd, int32_t offset, uint8_t base)
{
    return syscall(SYSCALL_SEEK,fd,offset,(uint32_t)(base));
}

int32_t unlink(const char *filepath)
{
    return syscall(SYSCALL_UNLINK,filepath);
}

int32_t mkdir(const char *path)
{
    return syscall(SYSCALL_MKDIR,path);
}

struct Dir *opendir(const char *path)
{
    return (struct Dir *)syscall(SYSCALL_OPENDIR,path);
}

int32_t closedir(struct Dir *dir)
{
    return syscall(SYSCALL_CLOSEDIR,dir);
}

int32_t rewinddir(struct Dir *dir)
{
    return syscall(SYSCALL_REWINDDIR,dir);
}

struct DirEntry *readdir(struct Dir *dir)
{
    return (struct DirEntry *)syscall(SYSCALL_READDIR,dir);
}

int32_t rmdir(const char *path)
{
    return syscall(SYSCALL_DIRREMOVE,path);
}

int32_t getcwd(char *buf)
{
    return syscall(SYSCALL_GETCWD,buf);
}

int32_t chdir(const char *dir)
{
    return syscall(SYSCALL_CHDIR,dir);
}
