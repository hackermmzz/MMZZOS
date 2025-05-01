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
    AllInit=1;
    while(1);
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