#include "../lib/kernel/print.h"
#include "init.h"
#include "thread.h"
#include "../userprog/process.h"
#include"../lib/stdio.h"
#include"../fs/file.h"
void idle_task_func(void*arg);
void test(void*arg);
void test1(void*arg);
int main(){
    //初始
    MMZZOS_Init();
    //创建闲置任务
    idle_task=ThreadCreate("idle_task",1,idle_task_func,0);
    //
    ProcessExe(test1,"pro1");
    ThreadCreate("test0",1,test,0);
    asm("sti");
    //初始化文件系统
    FileSystem_init();
    //
    /*
    extern void partition_format(struct partition*part);
    extern bool mount_partition(struct ListNode*node,int arg);
    extern struct partition*CurPartition;
    partition_format(CurPartition);//
    ListTraversal(&(partition_list),mount_partition,(int)CurPartition->name);
    */
   
    struct stat state;
    stat("/home",&state);
    printf("%d %d %d\n",state.inode,state.size,state.filetype);
    
    //
    while(1){
        uint16_t key=KeyBoardGet();
        if(key==CODE_BACKSPACE_DOWN){
            put_char('\b');
        }
        else if(key==CODE_CTRL_Z){
            put_str("kill the process!\n");
        }
        else if(key==CODE_CTRL_C){
            put_str("terminate the process!\n");
        }
        else if(key==CODE_ENTER_DOWN){
            put_char('\n');
        }
        else{
            uint8_t code=KeyBoardConvertTo(key);
            if(code!=(uint8_t)-1){
                put_char(code);
            }
        }
    }
    return 0;
} 
void idle_task_func(void*arg){
    while(1){
        ThreadBlock();
        asm volatile("sti;hlt ":::"memory");
    }
}
void test(void*arg){
    while(1){
       
    }
}
void test1(void*arg){
    while(1){
        
    }
}
