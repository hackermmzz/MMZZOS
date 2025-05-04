#include "../lib/kernel/print.h"
#include "init.h"
#include "thread.h"
#include "../userprog/process.h"
#include"../lib/stdio.h"
#include"../fs/file.h"
#include"../shell/shell.h"
bool AllInit=0;//标记系统是否初始化完成
int main(){
    //初始
    MMZZOS_Init();
    asm("sti");
    //初始化文件系统
    FileSystem_init();
    //把文件写入磁盘
    /*
    int fd=open("/test.txt",O_CREATE);
    char buf[]="WangLiHong is a clever boy and he will eran more than 50w money per year!\nCan you find WangLiHong?\nhe is handsome\nWangLiHong will make it!\nI will play YuanShen tomorrow!\n";
    write(fd,buf,sizeof(buf));
    close(fd);

    {
        int fd=open("/test.txt",O_CREATE);
        char buf[]="WangLiHong is a clever boy and he will eran more than 50w money per year!";
        write(fd,buf,sizeof(buf));
        close(fd);
    }
    {
        struct disk*dk=&ide_channel[0].devices[0];
        uint32_t sec=30;
        void*buf=malloc(sec*512);
        ide_read(dk,buf,500,sec);
        int fd=open("/cat",O_CREATE);
        int cnt=write(fd,buf,sec*512);
        printf("You have write %d!\n",cnt);
        close(fd);
        fd=open("/cat",O_RDONLY);
        void*buff=malloc(sec*512);
        ASSERT(read(fd,buff,cnt)==cnt);
        printf("memcpy res is:%d\n",memcmp(buf,buff,sec*512));
    }
    {
        struct disk*dk=&ide_channel[0].devices[0];
        uint32_t sec=30;
        void*buf=malloc(sec*512);
        ide_read(dk,buf,300,sec);
        int fd=open("/grep",O_CREATE);
        int cnt=write(fd,buf,sec*512);
        printf("You have write %d!\n",cnt);
        close(fd);
        fd=open("/grep",O_RDONLY);
        void*buff=malloc(sec*512);
        ASSERT(read(fd,buff,cnt)==cnt);
        printf("memcpy res is:%d\n",memcmp(buf,buff,sec*512));
    }
    while(1);
    */
    //
    AllInit=1;
    //退出主线程
    ThreadExit(RunningThread(),1);
    return 0;
} 

void init(void*arg){
    while(!AllInit);//这里直接简单粗暴等到系统初始化完成,也不用去调度啥的了,等后期实现了用户级线程yield再更改
    int32_t ret=fork();
    if(ret){
        while(1);
    }else{
        shell();
    }
}