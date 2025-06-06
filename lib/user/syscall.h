#ifndef MMZZ_SYSCALL_H
#define MMZZ_SYSCALL_H
#include "../stdint.h"
#include <stdarg.h>
////////////////////////////////////////
struct stat;
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
    SYSCALL_STAT,
    SYSCALL_FORK,
    SYSCALL_CLEAR,
    SYSCALL_PS,//获取系统目前所有的进程信息
    SYSCALL_EXEC,
    SYSCALL_WAIT,
    SYSCALL_EXIT,
    SYSCALL_PIPE,
    SYSCALL_FDREDIRECT,
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
int sleep(uint32_t msecond);
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
int32_t getcwd(char*buf,uint32_t buff_size);
int32_t chdir(const char*dir);
int stat(const char*path,struct stat*buf);
int32_t fork();
int clear();
int ps(void*buf);
int exec(const char*path,int32_t argc,char**argv);
pid_t wait(int32_t*status);
void exit(int code);
bool pipe(int fd[2]);
bool fd_redirect(int32_t old,int32_t new);
////////////////////////////////////////
#endif