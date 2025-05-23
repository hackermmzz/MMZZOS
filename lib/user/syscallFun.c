#include "syscall.h"
#include "../../fs/file.h"
#include "../../fs/dir.h"
#include"../../fs/super_block.h"
#include "../../fs/inode.h"
#include "../../device/keyboard.h"
#include"../../userprog/process.h"
#include "../kernel/print.h"
#include "../../kernel/thread.h"
#include "../../device/timer.h"
#include"../../userprog/wait_exit.h"
#include"../kernel/ioqueue.h"
/*
这里实现了所有的和系统调用相关的函数，把他单独放在这一个文件里面!!!这里实现了所有的和系统调用相关的函数，把他单独放在这一个文件里面!!!
这里实现了所有的和系统调用相关的函数，把他单独放在这一个文件里面!!!这里实现了所有的和系统调用相关的函数，把他单独放在这一个文件里面!!!
这里实现了所有的和系统调用相关的函数，把他单独放在这一个文件里面!!!这里实现了所有的和系统调用相关的函数，把他单独放在这一个文件里面!!!
*/
//////////////////////////////////////////////////
#define MIN(a,b)  ((a)<(b)?(a):(b))
#define MAX(a,b)  ((a)>(b)?(a):(b))
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
//判断fd文件描述符是否是管道
bool IsPipe(int32_t fd){
    fd=LocalFdToGlobalFd(fd);
    if(fd==-1)return 0;
    return GlobalFileTable[fd].flag==PIPE_FLAG;
}
//
bool syscall_pipe(int fd[2]){
    int32_t fd_g=GetFreeGlobalFd();
    struct FILE*file=&GlobalFileTable[fd_g];
    if(fd_g==-1)return 0;
    void*buf=mallocKernelPage(1);
    if(buf==0){
        file->inode=0;
        return 0;
    }
    file->inode=buf;
    file->flag=PIPE_FLAG;
    file->fd_pos=2;//读写各占一次
    //安装到进程
    fd[0]=ProcessInstallFd(fd_g);
    fd[1]=ProcessInstallFd(fd_g);
    if(fd[0]==-1||fd[1]==-1){
        struct PCB*pcb=RunningThread();
        if(fd[0]!=-1)pcb->fd[fd[0]]=-1;
        if(fd[1]!=-1)pcb->fd[fd[1]]=-1;
        file->inode=0;
        free_page(KERNEL,(uint32_t)buf,1);
        return 0;
    }
    //初始化管道
    struct ioQueue*queue=buf;
    ioQueueInit(queue);
    queue->len=PAGE_SIZE-sizeof(*queue);
    queue->buff=(uint8_t*)((uint32_t)buf+sizeof(*queue));
    return 1;
}

int32_t pipe_read(int32_t fd,void*buf,uint32_t cnt){
    fd=LocalFdToGlobalFd(fd);
    if(fd==-1)return 0;
    struct ioQueue*queue=(struct ioQueue*)(GlobalFileTable[fd].inode);
    uint32_t bufLen=ioQueueLen(queue);
    cnt=MIN(cnt,bufLen);//最多读取队列当前含有数据的数量
    byte*buf_=buf;
    uint32_t read=0;
    while(cnt--){
        *buf_=ioQueueGet(queue);
        ++buf_;
        ++read;
    }
    return read;
}
int32_t pipe_write(int32_t fd,const void*buf,uint32_t cnt){
    fd=LocalFdToGlobalFd(fd);
    if(fd==-1)return 0;
    struct ioQueue*queue=(struct ioQueue*)(GlobalFileTable[fd].inode);
    uint32_t bufLeft=queue->len-ioQueueLen(queue);
    cnt=MIN(cnt,bufLeft);//最多写入剩余的空间数量
    const byte*buf_=buf;
    uint32_t write=0;
    while(cnt--){
        ioQueuePut(queue,*buf_);
        ++buf_;
        ++write;
    }
    return write;
}
bool pipe_close(int32_t fd_l){
    if(fd_l<STD_FD_CNT||fd_l>+MAX_FILE_CNT_OPEN_PROCESS)return 0;
    struct PCB*pcb=RunningThread();
    int32_t fd_g=pcb->fd[fd_l];
    pcb->fd[fd_l]=-1;
    if(fd_g==-1)return 0;
    struct FILE*file=&GlobalFileTable[fd_g];
    if(--(file->fd_pos)==0){
        free_page(KERNEL,(uint32_t)file->inode,1);
        file->inode=0;//回收全局文件描述符
    }
    return 1;
}
uint32_t syscall_write(uint32_t fd,const byte*buf,uint32_t size){
    struct PCB*pcb=RunningThread();
    //如果是管道(标准输入输出流都可能被重定向为管道)
    if(IsPipe(fd)){
        return pipe_write(fd,buf,size);
    }
    //如果fd是标准流(注意输出流可能被重定向到一个文件)
    else if(fd==STDOUT_FD&&pcb->fd[fd]==-1){
         for(uint32_t i=0;i<size;++i){
            put_char(*buf);
            ++buf;
         }
         return size;
    }
    //否则就是普通文件的写入
    else if(pcb->fd[fd]!=-1){
        fd=LocalFdToGlobalFd(fd);
        if(fd==-1)return 0;
        //判断是否有写权限
        struct FILE*file=&GlobalFileTable[fd];
        //具有写入权限才可以写
        if(file->flag&O_WRONLY){
            return file_write(fd,buf,size);
        }
    }
    return 0;
}

int syscall_sleep(uint32_t msecond){
    //注意这里我们最小堆开的是1024个，所以很可能会sleep失败，这里会出现bug，但是鄙人觉得我的操作系统做不到跑1024个线程，所以才没改
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
    int32_t parent=-1;
    SearchFile(file,name,&entry,&parent,0);
    //如果连最后一层都没解析到
    if(parent==-1)return -1;
    bool found=entry.filetype==FT_FILE;//文件是否存在
     //如果文件不存在并且也没有创建的flag
    if(!found&&!(flag&O_CREATE))return -1;
    //如果已经存在了还去创建
    if(found&&(flag&O_CREATE))return -1;
    int32_t fd=-1;
    switch (flag&O_CREATE)
    {
    case O_CREATE:
        struct Dir*dir=DirOpen(CurPartition,parent);
        ASSERT(dir);
        fd=file_create(dir,name,flag);
        break;
    default:
        fd=file_open(entry.index,flag);
    }
    return fd;
}

int32_t syscall_close(int32_t fd_l){
    if(fd_l<0||fd_l>=MAX_FILE_CNT_OPEN_PROCESS)return 0;
    struct PCB*pcb=RunningThread();
    //如果是管道
    if(IsPipe(fd_l)){
        return pipe_close(fd_l);
    }else if(pcb->fd[fd_l]!=-1){
        int fd_g=LocalFdToGlobalFd(fd_l);
        pcb->fd[fd_l]=-1;//关闭局部描述符
        if(fd_g==-1)return 0;//关闭失败
        //
        struct FILE*file=&(GlobalFileTable[fd_g]);
        if(file->inode){
            if(InodeClose(file->inode))
            {
                file->inode=0;//回收全局描述符
            }
            return 1;//关闭成功
        }
    }
    return 0;
}
int32_t syscall_read(int32_t fd_l,void*buf,uint32_t bytes){
    struct PCB*pcb=RunningThread();
    if(IsPipe(fd_l)){
        return pipe_read(fd_l,buf,bytes);
    }
    //标准输入流
    else if(fd_l==STDIN_FD&&pcb->fd[fd_l]==-1){
        KeyBoardRead(buf,bytes);
        return bytes;
    }else if(pcb->fd[fd_l]!=-1){
        int fd=LocalFdToGlobalFd(fd_l);
        if(fd==-1)return 0;
        struct FILE*file=&GlobalFileTable[fd];
        //具有读取权限才可以读
        if(file->flag&O_RDONLY){
            return file_read(fd,buf,bytes);
        }
    }
    return 0;
}
///////////////////////////////////////////////////

int32_t syscall_seek(int32_t fd,int32_t offset,uint32_t flag){
    fd=LocalFdToGlobalFd(fd);
    if(fd==-1)return -1;
    int32_t fileSize=GlobalFileTable[fd].inode->size;
    int32_t pos=GlobalFileTable[fd].fd_pos;
    int32_t newpos;
    if(flag==SEEK_SET)newpos=offset;
    else if(flag==SEEK_CUR)newpos=pos+offset;
    else if(flag==SEEK_END)newpos=fileSize+offset;
    if(newpos<0||newpos>fileSize){
        return -1;
    }
    GlobalFileTable[fd].fd_pos=newpos;
    return newpos;
}

int32_t syscall_unlink(const char*file){
    //先检查文件是否存在
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct DirEntry entry;
    struct Dir*dir=SearchFile(file,name,&entry,&parent,1);
    //如果不存在或者不是普通文件直接返回
    if(entry.filetype!=FT_FILE)
    {
        if(dir)DirClose(dir);//可能打开的是目录而不是文件
        return 0;
    }
    //先检查文件是否已经被打开
    if(dir->inode->open_cnt>=2){
        DirClose(dir);
        return 0;
    }
    
    //回收掉inode
    int addrCnt=(12+BLOCK_SIZE/4);
    uint32_t*addr=syscall_malloc(addrCnt*sizeof(uint32_t));
    if(addr==0){
        DirClose(dir);
        return 0;
    }
    ASSERT(InodeRecycle(CurPartition,dir->inode,addr));
    //删除在父目录的记录
    struct Dir*fa=DirOpen(CurPartition,parent);
    ASSERT(fa);
    ASSERT(DeleteDirectoryDirEntry(CurPartition,fa,name,addr));//删不掉属于系统级别的硬伤，直接卡死
    DirClose(fa);
    DirClose(dir);
    syscall_free(addr);
    return 1;
}
int32_t syscall_mkdir(const char*path){
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent,1);
    //如果都没解析到最后一层,或者解析到最后一层但是存在对应的目录项，直接返回
    if(parent==-1||entry.filetype!=FT_UNKOWN){
        if(dir)DirClose(dir);
        return 0;
    }
    //创建3个目录项,分别是path下的. .. 和path父目录下的path
    int idx=-1;
    idx=InodeBitmapAllocate(CurPartition);
    if(idx==-1) goto MkDirRollBack;
    //path
    entry.index=idx;
    memcpy(entry.filename,name,FILENAME_MAX_LEN);
    entry.filetype=FT_DIR;
    if(!CreateDirEntry(CurPartition,dir,&entry))goto MkDirRollBack;
    DirClose(dir);//关闭目录
    dir=DirOpen(CurPartition,idx);//打开新建的目录
    ASSERT(dir);
    InodeInit(dir->inode,idx);
    dir->inode->open_cnt=1;
    //.
    entry.index=dir->inode->index;
    memcpy(entry.filename,".",2);
    entry.filetype=FT_DIR;
    if(!CreateDirEntry(CurPartition,dir,&entry))
    {
        goto MkDirRollBack;
    }
    //..
    entry.index=parent;//..指向父亲节点
    memcpy(entry.filename,"..",3);
    entry.filetype=FT_DIR;
    if(!CreateDirEntry(CurPartition,dir,&entry))goto MkDirRollBack;
    //同步到磁盘
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
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent,1);
    //如果不存在
    if(parent==-1||entry.filetype!=FT_DIR){
        if(dir)DirClose(dir);
        return 0;
    }
    return dir;
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
    int addrCnt=(12+BLOCK_SIZE/4);
    uint32_t*addr=syscall_malloc(addrCnt*sizeof(uint32_t));
    if(addr==0)return 0;
    memcpy(addr,dir->inode->i_sectors,48);
    if(dir->inode->i_sectors[12])
    ide_read(CurPartition->my_disk,(&addr[0])+12,dir->inode->i_sectors[12],BLOCK_OCCUPY_SECTOR);
    int32_t tarpos=dir->dir_pos;
    int32_t idx0=0;
    int32_t curpos=0;
    int32_t cnt=BLOCK_SIZE/sizeof(struct DirEntry);
    while(curpos<=tarpos){
        if(addr[idx0]==0){
            ++idx0;
            continue;
        }
        //
        ide_read(CurPartition->my_disk,dir->dir_buf,addr[idx0],BLOCK_OCCUPY_SECTOR);
        struct DirEntry*entrys=(struct DirEntry*)dir->dir_buf;
        for(int i=0;i<cnt;++i){
            struct DirEntry*entry=&entrys[i];
            if(entry->filetype!=FT_UNKOWN){
                if(curpos==tarpos){
                    ++(dir->dir_pos);
                    syscall_free(addr);
                    return entry;
                }
                ++curpos;
            }
        }
        ++idx0;
    }
    syscall_free(addr);
    return 0;
}
//
int32_t syscall_rmdir(const char*path){
    //先判断目录是否存在
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent,1);
    //如果该目录不存在或者不是目录
    if(parent==-1||entry.filetype!=FT_DIR){
        if(dir)DirClose(dir);
        return 0;
    }
    //判断要删除的目录是否被打开了(因为我们自己打开了一次，所以打开次数不能超过2),如果被打开了，那么是不能删除的
    if(dir->inode->open_cnt>=2)
    {
        DirClose(dir);
        return 0;
    }
    //判断要删除的那个目录是否为空
    if(dir->inode->size!=2){
        DirClose(dir);
        return 0;
    }
    //删除在父目录的记录
    struct Dir*fa=DirOpen(CurPartition,parent);
    int addrCnt=(12+BLOCK_SIZE/4);
    uint32_t*addr=syscall_malloc(addrCnt*sizeof(uint32_t));
    ASSERT(DeleteDirectoryDirEntry(CurPartition,fa,name,addr));
    //回收掉inode
    ASSERT(InodeRecycle(CurPartition,dir->inode,addr));
    DirClose(fa);
    DirClose(dir);
    syscall_free(addr);
    return 1;
}
//
int32_t syscall_getcwd(char*buf,uint32_t buff_size){
    *buf=0;
    struct PCB*pcb=RunningThread();
    struct Dir* dir=pcb->workDir;
    ASSERT(dir);
    if(dir==&rootDir){
        buf[0]='/';
        buf[1]=0;
        return 1;
    }
    //先获取当前目录的父目录编号
    int fa=DirGetParentIndex(CurPartition,dir,dir->dir_buf);
    //
    char*tmp=buf+buff_size-1;
    int cur=dir->inode->index;
    int addrCnt=(12+BLOCK_SIZE/4);
    uint32_t*addr=syscall_malloc(addrCnt*sizeof(uint32_t));
    if(addr==0)return 0;
    char name[FILENAME_MAX_LEN+1];
    //只要当前目录不是根目录就一直往上找
    while(cur){
        int faa;
        DirGetIndexName(CurPartition,fa,cur,addr,name,&faa);
        cur=fa;
        fa=faa;
        int len=strlen(name);
        while(len--)*tmp=name[len],--tmp;
        *tmp='/';
        --tmp;
    }
    //把字符串前移
    int len=0;
    char*tmpp=buf,*end=buf+buff_size;
    ++tmp;
    while(tmp!=end)*tmpp=*tmp,++tmpp,++tmp,++len;
    *tmpp=0;
    syscall_free(addr);
    return len;
}
//
int32_t syscall_chdir(const char*path){
    //先判断目录是否存在
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent,1);
    //如果该目录不存在或者根本不是目录
    if(parent==-1||entry.filetype!=FT_DIR)
    {
        if(dir)DirClose(dir);
        return 0;
    }
    //关闭原目录
    struct PCB*pcb=RunningThread();
    ASSERT(pcb->workDir);
    DirClose(pcb->workDir);
    //打开新目录
    pcb->workDir=dir;
    return 1;
}
//
int syscall_stat(const char*path,struct stat*buf){
    //先判断目录是否存在
    struct DirEntry entry;
    char name[FILENAME_MAX_LEN+1];
    int32_t parent=-1;
    struct Dir*dir=SearchFile(path,name,&entry,&parent,1);
    //如果该目录不存在
    if(parent==-1)
    {
        if(dir)DirClose(dir);
        return 0;
    }
    //
    buf->filetype=entry.filetype;
    buf->inode=entry.index;
    buf->size=dir->inode->size;
    DirClose(dir);
    return 1;
}
//
int syscall_clear(){
    Clear_Screen();
    return 1;
}
//
int syscall_ps(void*buf){
    struct ListNode*cur=(ThreadList.head.nxt);
    struct ListNode*end=&(ThreadList.tail);
    struct ProcessInfo*info=(struct ProcessInfo*)buf;
    int cnt=0;
    while(cur!=end){
        struct PCB*pcb=elem2entry(struct PCB,tag,cur);
        //
        info->pid=pcb->pid;
        info->parent=pcb->pa_pid;
        info->status=pcb->status;
        info->totalTicks=pcb->totalTicks;
        strcpy(info->name,pcb->name);
        ++cnt;
        //
        ++info;
        cur=cur->nxt;
    }
    return cnt;
}
//
bool syscall_fd_redirect(int32_t old,int32_t new){
    struct PCB*pcb=RunningThread();
    if(old<=STD_FD_CNT&&new==-1)
    {
        pcb->fd[old]=-1;
        return 1;
    }
    if(old<0||old>=MAX_FILE_CNT_OPEN_PROCESS||new<0||new>=MAX_FILE_CNT_OPEN_PROCESS)return 0;
    //关闭原来的文件
    syscall_close(old);
    //
    int32_t fd_g=pcb->fd[new];
    pcb->fd[old]=fd_g;
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
    extern int32_t syscall_fork();
    extern int32_t syscall_exec(const char*path,int32_t argc,char*argv[]);
    ///////////////////////////////////////////////////
    MakeSyscallTable(SYSCALL_GETPID,(uint32_t)(void*)syscall_GetProcessPid,0);
    MakeSyscallTable(SYSCALL_MALLOC,(uint32_t)(void*)syscall_malloc,1);
    MakeSyscallTable(SYSCALL_FREE,(uint32_t)(void*)syscall_free,1);
    MakeSyscallTable(SYSCALL_SLEEP,(uint32_t)(void*)syscall_sleep,1);
    MakeSyscallTable(SYSCALL_WRITE,(uint32_t)(void*)syscall_write,3);
    MakeSyscallTable(SYSCALL_OPEN,(uint32_t)(void*)syscall_open,2);
    MakeSyscallTable(SYSCALL_CLOSE,(uint32_t)(void*)syscall_close,1);
    MakeSyscallTable(SYSCALL_READ,(uint32_t)(void*)syscall_read,3);
    MakeSyscallTable(SYSCALL_SEEK,(uint32_t)(void*)syscall_seek,3);
    MakeSyscallTable(SYSCALL_UNLINK,(uint32_t)(void*)syscall_unlink,1);
    MakeSyscallTable(SYSCALL_MKDIR,(uint32_t)(void*)syscall_mkdir,1);
    MakeSyscallTable(SYSCALL_OPENDIR,(uint32_t)(void*)syscall_opendir,1);
    MakeSyscallTable(SYSCALL_CLOSEDIR,(uint32_t)(void*)syscall_closedir,1);
    MakeSyscallTable(SYSCALL_READDIR,(uint32_t)(void*)syscall_readdir,1);
    MakeSyscallTable(SYSCALL_REWINDDIR,(uint32_t)(void*)syscall_rewinddir,1);
    MakeSyscallTable(SYSCALL_DIRREMOVE,(uint32_t)(void*)syscall_rmdir,1);
    MakeSyscallTable(SYSCALL_GETCWD,(uint32_t)(void*)syscall_getcwd,2);
    MakeSyscallTable(SYSCALL_CHDIR,(uint32_t)(void*)syscall_chdir,1);
    MakeSyscallTable(SYSCALL_STAT,(uint32_t)(void*)syscall_stat,2);
    MakeSyscallTable(SYSCALL_FORK,(uint32_t)(void*)syscall_fork,0);
    MakeSyscallTable(SYSCALL_CLEAR,(uint32_t)(void*)syscall_clear,0);
    MakeSyscallTable(SYSCALL_PS,(uint32_t)(void*)syscall_ps,1);
    MakeSyscallTable(SYSCALL_EXEC,(uint32_t)(void*)syscall_exec,3);
    MakeSyscallTable(SYSCALL_WAIT,(uint32_t)(void*)syscall_wait,1);
    MakeSyscallTable(SYSCALL_EXIT,(uint32_t)(void*)syscall_exit,1);
    MakeSyscallTable(SYSCALL_PIPE,(uint32_t)(void*)syscall_pipe,1);
    MakeSyscallTable(SYSCALL_FDREDIRECT,(uint32_t)(void*)syscall_fd_redirect,1);
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