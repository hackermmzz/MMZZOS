#ifndef MMZZOS_PRINT_H
#define MMZZOS_PRINT_H
#include "../stdint.h"
extern void  put_char(uint8_t chr);
extern void  Clear_Screen();//清空屏幕并把光标置于开头
void put_str(const char*);//输出字符串
void put_num(int);
void put_hex(uint32_t);
void printk(const char*format,...);
#endif