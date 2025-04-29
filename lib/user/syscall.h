#ifndef MMZZ_SYSCALL_H
#define MMZZ_SYSCALL_H
#include "../stdint.h"
#include <stdarg.h>
#include "../kernel/print.h"
#include "../../kernel/thread.h"
#include "../../device/timer.h"
////////////////////////////////////////
////////////////////////////////////////
enum SYSCALL_CODE{
    SYSCALL_GETPID,
    SYSCALL_OPEN,
    SYSCALL_READ,
    SYSCALL_CLOSE,
    SYSCALL_SEEK,
    SYSCALL_UNLINK,
    SYSCALL_WRITE,
    SYSCALL_MALLOC,
    SYSCALL_FREE,
    SYSCALL_SLEEP,
    SYSCALL_MKDIR,
    SYSCALL_OPENDIR,
    SYSCALL_CLOSEDIR,
    SYSCALL_REWINDDIR,
    SYSCALL_READDIR,
    SYSCALL_DIRREMOVE,
    SYSCALL_GETCWD,
    SYSCALL_CHDIR,
};//系统调用号
////////////////////////////////////////
void syscall_init();
int syscall(int number,...);
int Syscall0(int);//0个参数
int Syscall1(int,int);//1个参数
int Syscall2(int,int,int);//2个参数
int Syscall3(int,int,int,int);//3个参数
///////////////////////////////////////
uint32_t getpid();
uint32_t write(uint32_t fd,const byte*buf,uint32_t size);
uint32_t read(uint32_t fd,void*buf,uint32_t size);
void*malloc(uint32_t size);
void free(void*addr);
void sleep(uint32_t msecond);
int32_t open(const char*filename,uint8_t flag);
int32_t close(int32_t fd);
int32_t seek(int32_t fd,int32_t offset,uint8_t base);
int32_t unlink(const char*filepath);
int32_t mkdir(const char*path);
struct Dir*opendir(const char*path);
int32_t closedir(struct Dir*dir);
int32_t rewinddir(struct Dir*dir);
struct DirEntry* readdir(struct Dir*dir);
int32_t rmdir(const char*path);
int32_t getcwd(char*buf);
int32_t chdir(const char*dir);
////////////////////////////////////////
#endif