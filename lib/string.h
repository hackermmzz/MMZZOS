#ifndef MMZZ_STRING_H
#define MMZZ_STRING_H
#include "stdint.h"
/*
实现一些字符串函数
*/
/////////////////////////////
uint32_t strlen(const char*str);
char* strcpy(char*dst,const char*src);
int strcmp(const char*s0,const char*s1);
int32_t reverse(char*str);
void* memset(void*dst,int data,uint32_t size);
void* memcpy(void*dst,const void*src,uint32_t size);
/////////////////////////////
#endif