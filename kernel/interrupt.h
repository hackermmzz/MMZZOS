#ifndef MMZZOS_INTERRUPT_H
#define MMZZOS_INTERRUPT_H
/*
中断相关
*/
#include "global.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/string.h"
/////
#define IDT_DESC_CNT 0x80      //中断个数
/////
extern uint32_t interrupt_entry_table [IDT_DESC_CNT+1];//中断入口地址表
extern uint32_t interrupt_table[IDT_DESC_CNT+1];//中断处理函数
extern const char* interrupt_name[IDT_DESC_CNT+1];//中断的名称
////
struct GateDesc{
    uint16_t function_offset_low;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attr;
    uint16_t function_offset_high;
};
extern struct GateDesc IDT_DESC[IDT_DESC_CNT+1];
extern void syscall_handler();
void makeIDT_Desc(struct GateDesc*desc,uint8_t attr,uint32_t func);
void IDT_DESC_Init();
void PIC_Init();
void idt_init();
void general_interrupt_process(uint8_t code);//对于绝大部分中断采用默认处理方式
void interrupt_enable();
void interrupt_disable();
uint8_t interrupt_status();
////
#endif