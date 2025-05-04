#ifndef MMZZ_FILE_H
#define MMZZ_FILE_H
#include "inode.h"
#include "dir.h"
////////////////////////////////////
#define MAX_FILE_CNT_SYSTEM_OPEN 64//系统可打开的最多文件数量
#define MAX_PATH_LEN 256//路径最长长度
#define PIPE_FLAG 0xffff //用于标志文件类型为管道
////////////////////////////////////
struct FILE{
    uint32_t fd_pos; 
    uint32_t flag;
    struct Inode*inode;
};


enum STD_FD{
    STDOUT_FD=0,//标准输出流
    STDIN_FD,//标准输入流
    STD_FD_CNT,//占位符
};

enum FS_BITMAP_TYPE{
    BITMAP_FOR_INODE,
    BITMAP_FOR_BLOCK
};

enum {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREATE=4,
};


/////////////////////////////////////////////////
extern struct FILE GlobalFileTable[MAX_FILE_CNT_SYSTEM_OPEN];
/////////////////////////////////////////////////
int32_t InodeBitmapAllocate(struct partition*part);
int32_t BlockBitmapAllocate(struct partition*part);
void BitMapUpdate(struct partition*part,uint32_t idx,enum FS_BITMAP_TYPE flag);
int32_t file_create(struct Dir*dir,const char*file,uint8_t flag);
int32_t file_open(uint32_t index,uint8_t flag);
uint32_t file_write(int32_t fd,const void*buf,uint32_t size);
int32_t LocalFdToGlobalFd(int32_t fd_l);
int32_t file_read(int32_t fd,void*buf,uint32_t cnt);
int32_t DirGetParentIndex(struct partition*part,struct Dir*dir,void*buf);
void DirGetIndexName(struct partition*part,int32_t parent,int32_t index,uint32_t*addr_buf,char*name,int32_t*fa);
int32_t GetFreeGlobalFd();
int32_t ProcessInstallFd(int16_t global_fd);
bool IsPipe(int32_t fd);
////////////////////////////////////
#endif