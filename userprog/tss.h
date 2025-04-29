#ifndef MMZZ_TSS_H
#define MMZZ_TSS_H
#include "../kernel/global.h"
#include "../lib/stdint.h"
#include "../kernel/thread.h"
//////////////////////////////////////
struct Tss{
    uint32_t backlink;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t io_base;
};
extern struct Tss userTss;
void TssUpdateEsp(struct PCB*pcb);
void Tss_init();
struct gdt_desc MakeGDT_DESC(uint32_t* desc_addr,uint32_t limit,uint8_t attr_low,uint8_t attr_high);
//////////////////////////////////////
#endif