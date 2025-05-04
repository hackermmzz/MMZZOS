#include"syscall.h"

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
    return Syscall0(SYSCALL_GETPID);
}

uint32_t write(uint32_t fd, const byte *buf, uint32_t size)
{
    return Syscall3(SYSCALL_WRITE,fd,(int)buf,size);
}

uint32_t read(uint32_t fd, void*buf, uint32_t size)
{
    return Syscall3(SYSCALL_READ,fd,(int)buf,size);
}

void *malloc(uint32_t size)
{
    return (void*)Syscall1(SYSCALL_MALLOC,size);
}

void free(void *addr)
{
    Syscall1(SYSCALL_FREE,(int)addr);
}

int sleep(uint32_t msecond)
{
    return Syscall1(SYSCALL_SLEEP,msecond);
}

int32_t open(const char *filename, uint8_t flag)
{
    return Syscall2(SYSCALL_OPEN,(int)filename,(uint32_t)flag);
}

int32_t close(int32_t fd)
{
    return Syscall1(SYSCALL_CLOSE,fd);
}

int32_t seek(int32_t fd, int32_t offset, uint8_t base)
{
    return Syscall3(SYSCALL_SEEK,fd,offset,(uint32_t)(base));
}

int32_t unlink(const char *filepath)
{
    return Syscall1(SYSCALL_UNLINK,(int)filepath);
}

int32_t mkdir(const char *path)
{
    return Syscall1(SYSCALL_MKDIR,(int)path);
}

struct Dir *opendir(const char *path)
{
    return (struct Dir *)Syscall1(SYSCALL_OPENDIR,(int)path);
}

int32_t closedir(struct Dir *dir)
{
    return Syscall1(SYSCALL_CLOSEDIR,(int)dir);
}

int32_t rewinddir(struct Dir *dir)
{
    return Syscall1(SYSCALL_REWINDDIR,(int)dir);
}

struct DirEntry *readdir(struct Dir *dir)
{
    return (struct DirEntry *)Syscall1(SYSCALL_READDIR,(int)dir);
}

int32_t rmdir(const char *path)
{
    return Syscall1(SYSCALL_DIRREMOVE,(int)path);
}

int32_t getcwd(char *buf,uint32_t buff_size)
{
    return Syscall2(SYSCALL_GETCWD,(int)buf,buff_size);
}

int32_t chdir(const char *dir)
{
    return Syscall1(SYSCALL_CHDIR,(int)dir);
}


int stat(const char *path, struct stat* buf)
{
    return Syscall2(SYSCALL_STAT,(int)path,(int)buf);
}

int clear(){
    return Syscall0(SYSCALL_CLEAR);
}

int ps(void *buf)
{
    return Syscall1(SYSCALL_PS,(int)buf);
}

int exec(const char *path, int32_t argc, char **argv)
{
    return Syscall3(SYSCALL_EXEC,(int)path,argc,(int)argv);
}

pid_t wait(int32_t *status)
{
    return Syscall1(SYSCALL_WAIT,(int)status);
}

void exit(int status)
{
    Syscall1(SYSCALL_EXIT,(int)(int8_t)status);
}

bool pipe(int fd[2])
{
    return Syscall1(SYSCALL_PIPE,(int)fd);
}

bool fd_redirect(int32_t old, int32_t new)
{
    return Syscall2(SYSCALL_FDREDIRECT,old,new);
}
