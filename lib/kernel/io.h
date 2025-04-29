#ifndef MMZZ_IO_H
#define MMZZ_IO_H
#include "../stdint.h"
/////////////////////////////////////
//向指定端口写入一个字节
static inline void outB(uint16_t port,uint8_t data){
    asm volatile("outb %b0,%w1"::"a"(data),"Nd"(port));
}

//循环写入指定数量的字到指定端口
static inline void outSW(uint16_t port,const void*addr,uint32_t cnt){
    asm volatile("cld; rep outsw":"+S"(addr),"+c"(cnt):"d"(port));
}

//读取指定端口一个字节
static inline uint8_t inB(uint16_t port){
    uint8_t ret;
    asm volatile("inb %w1,%b0":"=a"(ret):"Nd"(port));
    return ret;
}

//循环从指定端口读取指定数量的字
static inline void inSW(uint16_t port,void*buf,uint32_t cnt){
    asm volatile("cld;rep insw":"+D"(buf),"+c"(cnt):"d"(port):"memory");
}
/////////////////////////////////////
#endif