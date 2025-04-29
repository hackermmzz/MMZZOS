#include "init.h"

void MMZZOS_Init()
{
    //清空屏幕
    Clear_Screen();
    //
    put_str("Init MMZZOS\n");
    //初始化中断
    idt_init();
    //初始化内存池
    mem_init();
    //初始化时钟中断
    timer_init();
    //初始化内核主线程
    init_thread();
    //初始化键盘
    KeyBoardInit();
    //初始化tss表
    Tss_init();
    //初始化系统调用表
    syscall_init();
    //初始化硬盘驱动
    ide_init();
}