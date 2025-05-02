#include"interrupt.h"
#include "../lib/kernel/io.h"
#include "../device/timer.h"
#include "../device/keyboard.h"
#include "../device/ide.h"
///////////////////////////////////////////
#define EFLAGS_IF 0x00000200
///////////////////////////////////////////
#define PIC_M_CTRL 0x20	       // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21	       // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0	       // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1	       // 从片的数据端口是0xa1
////////////////////////////////////////////
struct GateDesc IDT_DESC[IDT_DESC_CNT+1];
uint32_t interrupt_table[IDT_DESC_CNT+1];
uint8_t IDT_Privilage[IDT_DESC_CNT+1];//中断权限级
const char* interrupt_name[IDT_DESC_CNT+1];
///////////////////////////////////////////
void makeIDT_Desc(struct GateDesc*desc,uint8_t attr,uint32_t func){
    desc->attr=attr;
    desc->function_offset_low=func&(0xffff);
    desc->function_offset_high=(func>>16)&(0xffff);
    desc->dcount=0;
    desc->selector=SELECTOR_K_CODE;
}
void IDT_DESC_Init()
{
    for(int i=0;i<=IDT_DESC_CNT;++i)
    {
        IDT_Privilage[i]=IDT_DESC_ATTR_DPL0;//所有中断默认权限级均为0
        interrupt_table[i]=(uint32_t)general_interrupt_process;
    }
    //
    ///////////////////////对个别中断单独实现中断处理函数
    //时钟中断处理函数
    interrupt_table[0x20]=(uint32_t)(void*)timer_interrupt;
    //键盘中断处理函数
    interrupt_table[0x21]=(uint32_t)(void*)keyboard_interrupt;
    //系统调用处理函数(这个需要拿入口函数进行注册)
    interrupt_entry_table[0x80]=(uint32_t)(void*)syscall_handler;
    IDT_Privilage[0x80]=IDT_DESC_ATTR_DPL3;
    //磁盘读取中断处理函数
    interrupt_table[0x2e]=interrupt_table[0x2f]=(uint32_t)(void*)disk_itr_handler;
    /////////////////////////
    for(int i=0;i<=IDT_DESC_CNT;++i){
        interrupt_name[i]="Unkown";
        makeIDT_Desc(&IDT_DESC[i],IDT_Privilage[i],(uint32_t)interrupt_entry_table[i]);
    }
    //对部分中断起名字
    interrupt_name[0] = "#DE Divide Error";
    interrupt_name[1] = "#DB Debug Exception";
    interrupt_name[2] = "NMI Interrupt";
    interrupt_name[3] = "#BP Breakpoint Exception";
    interrupt_name[4] = "#OF Overflow Exception";
    interrupt_name[5] = "#BR BOUND Range Exceeded Exception";
    interrupt_name[6] = "#UD Invalid Opcode Exception";
    interrupt_name[7] = "#NM Device Not Available Exception";
    interrupt_name[8] = "#DF Double Fault Exception";
    interrupt_name[9] = "Coprocessor Segment Overrun";
    interrupt_name[10] = "#TS Invalid TSS Exception";
    interrupt_name[11] = "#NP Segment Not Present";
    interrupt_name[12] = "#SS Stack Fault Exception";
    interrupt_name[13] = "#GP General Protection Exception";
    interrupt_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    interrupt_name[16] = "#MF x87 FPU Floating-Point Error";
    interrupt_name[17] = "#AC Alignment Check Exception";
    interrupt_name[18] = "#MC Machine-Check Exception";
    interrupt_name[19] = "#XF SIMD Floating-Point Exception";
    //
    put_str("idt_desc_init finished!\n");
}

void PIC_Init()
{
    outB (PIC_M_CTRL, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
    outB (PIC_M_DATA, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
    outB (PIC_M_DATA, 0x04);   // ICW3: IR2接从片. 
    outB (PIC_M_DATA, 0x01);   // ICW4: 8086模式, 正常EOI

    /* 初始化从片 */
    outB (PIC_S_CTRL, 0x11);    // ICW1: 边沿触发,级联8259, 需要ICW4.
    outB (PIC_S_DATA, 0x28);    // ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
    outB (PIC_S_DATA, 0x02);    // ICW3: 设置从片连接到主片的IR2引脚
    outB (PIC_S_DATA, 0x01);    // ICW4: 8086模式, 正常EOI
    
    outB (PIC_M_DATA, 0xf8);
    outB (PIC_S_DATA, 0xbf);

    put_str("pic_init finished\n");
}

void idt_init()
{
    put_str("idt init start!\n");
    IDT_DESC_Init();
    PIC_Init();
    //加载idt
    uint64_t operand=(sizeof(IDT_DESC)-1)|(((uint64_t)(uint32_t)IDT_DESC)<<16);
    asm volatile("lidt %0"::"m"(operand));
    put_str("idt init finished\n");
}

void general_interrupt_process(uint8_t code)
{
    if(code==0x27||code==0x2f){//这两个中断不必理会
        return;
    }
    struct PCB*pcb=RunningThread();
    struct InterruptStack*stack=(struct InterruptStack*)(((uint32_t)pcb+PAGE_SIZE)-sizeof(struct InterruptStack));
    printk("Error Code:%d\nError Name:%s\nEip:%x\nEsp:%x\n",code,interrupt_name[code],stack->eip,stack->esp);
    PANIC_MACRO("HERE!\n");
}

void interrupt_enable()
{
    asm volatile("sti");
}

void interrupt_disable()
{
    asm volatile("cli":::"memory");
}

uint8_t interrupt_status()
{
    uint32_t flag;
    asm volatile("pushfl;popl %0":"=g"(flag));
    return (flag&EFLAGS_IF)?1:0;
}
