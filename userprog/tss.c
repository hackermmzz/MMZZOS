#include "tss.h"
struct Tss userTss;
void TssUpdateEsp(struct PCB *pcb)
{
    userTss.esp0=(uint32_t)(((uint32_t)(pcb))+PAGE_SIZE);
}
///////////////////////////////////////
void Tss_init()
{
    put_str("Tss init start!\n");
    //////////////////////////////////
    memset(&userTss,0,sizeof(userTss));
    userTss.io_base=sizeof(userTss);//禁用io位图
    userTss.ss0=SELECTOR_K_STACK;//0级栈初始化为内核的栈
    //////配置GDT表
    //配置tss表位置
    *(struct gdt_desc*)(0xc0000623)=MakeGDT_DESC((uint32_t*)(&userTss),sizeof(userTss)-1,TSS_ATTR_LOW,TSS_ATTR_HIGH);
    //给用户进程配置一个代码段和一个数据段
    *(struct gdt_desc*)(0xc000062b)=MakeGDT_DESC((uint32_t*)0,0xfffff,GDT_CODE_ATTR_LOW_DPL3,GDT_ATTR_HIGH);
    *(struct gdt_desc*)(0xc0000633)=MakeGDT_DESC((uint32_t*)0,0xfffff,GDT_DATA_ATTR_LOW_DPL3,GDT_ATTR_HIGH);
    //重新加载GDT
    uint64_t operand =((8*7 - 1) | ((uint64_t)(uint32_t)0xc0000603 << 16));
    asm volatile("lgdt %0" :: "m"(operand));
    asm volatile("ltr %w0" :: "r"(SELECTOR_TSS));
    //////////////////////////////////
    put_str("Tss init done!\n");
}

struct gdt_desc MakeGDT_DESC(uint32_t *desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high)
{
    struct gdt_desc ret;
    uint32_t desc_base = (uint32_t) desc_addr;
    ret.limit_low_word =  limit & 0x0000ffff;
    ret.base_low_word = desc_base & 0x0000ffff;
    ret.base_mid_byte = ((desc_base & 0x00ff0000) >> 16);
    ret.attr_low_byte = (uint8_t)(attr_low);
    ret.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high));
    ret.base_high_byte = desc_base >> 24;
    return ret;
}
