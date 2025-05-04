#ifndef MMZZ_STDIO_H
#define MMZZ_STDIO_H
#include "stdint.h"
#include "user/syscall.h"
#include<stdarg.h>
#include"string.h"
/////////////////////////////////////
int vsprintf(char*buf,const char*format,va_list arg);
int printf(const char*format,...);
int getchar();
int getline(void*buf,int32_t size);//获取一行最多size个输入直到遇到\n为止
int sprintf(char*buf,const char*format,...);
int putchar(int ch);
/////////////////////////////////////
#endif