#include "string.h"

uint32_t strlen(const char *str)
{
    uint32_t len=0;
    while(*str)++len,++str;
    return len;
}

char* strcpy(char *dst, const char *src)
{
    while(*src){
        *dst=*src;
        ++src;
        ++dst;
    }
    *dst=0;
    return dst;
}

char* strcat(char *dst, const char *src)
{
   int len=strlen(dst);
   dst+=len;
   while(*src){
    *dst=*src;
    ++src;
    ++dst;
   }
   *dst=0;
   return dst;
}

int strcmp(const char *s0, const char *s1)
{
    while(*s0&&*s1){
        if(*s0<*s1)return -1;
        else if(*s0>*s1)return 1;
        ++s0;++s1;
    }
    if(*s0==0&&*s1==0)return 0;
    if(*s0)return 1;
    return -1;
}
const char *strchr(const char *str, int ch)
{
    while(*str){
        if(*str==ch)return str;
        ++str;
    }
    return 0;
}
const char *strip(const char *str)
{
    while(*str&&*str==' ')++str;
    return str;
}
// 把字符串反转,返回值是字符串的长度
int32_t reverse(char *str)
{
    int32_t len=strlen(str);
    uint8_t*head=str,*tail=str+len-1,tmp;
    while(head<tail){
        tmp=*head;
        *head=*tail;
        *tail=tmp;
        ++head;
        --tail;
    }
    str[len]=0;
    return len;
}

void *memset(void *dst, int dt, uint32_t size)
{
    dt&=255;//只取最低一个字节
    uint8_t data=dt;
    //将起始地址4字节对齐
    while(((uint32_t)dst&0x3)&&size){
        *(uint8_t*)dst=data;
        dst=(void*)((uint32_t)(dst))+1;
        --size;
    }
    //加速
    uint32_t dat=(dt<<24)|(dt<<16)|(dt<<8)|dt;
    uint32_t buf=(uint32_t)dst;
    while(size>=4){
        *(uint32_t*)buf=dat;
        buf+=4;
        size-=4;
    }
    //补齐末尾字节
    while(size){
        *(uint8_t*)buf=data;
        ++buf;
        --size;
    }
    return 0;
}

void* memcpy(void *dst, const void *src, uint32_t size)
{
    //如果dst和src对4取模不相等，直接字节cpy
    if((((uint32_t)(dst))&0x3)!=(((uint32_t)(src))&0x3)){
        while(size--){
            *(uint8_t*)dst=*(uint8_t*)src;
            dst=(void*)((uint32_t)(dst))+1;
            src=(void*)((uint32_t)(src))+1;
        }
        return dst;
    }
     //将起始地址4字节对齐
     while(((uint32_t)dst&0x3)&&size){
        *(uint8_t*)dst=*(uint8_t*)src;
        dst=(void*)((uint32_t)(dst))+1;
        src=(void*)((uint32_t)(src))+1;
        --size;
    }
    //加速
    uint32_t d0=(uint32_t)dst,d1=(uint32_t)src;
    while(size>=4){
        *(uint32_t*)d0=*(uint32_t*)d1;
        d0+=4;
        d1+=4;
        size-=4;
    }
    //补齐末尾
    while(size){
        *(uint8_t*)d0=*(uint8_t*)d1;
        ++d0;
        ++d1;
        --size;
    }
    return dst;
}

int memcmp(const void *src0, const void *src1, int cnt)
{
    byte*s0=(byte*)src0;
    byte*s1=(byte*)src1;
    for(int i=0;i<cnt;++i,++s0,++s1){
        uint8_t a=*s0;
        uint8_t b=*s1;
        if(a<b)return -1;
        else if(a>b)return 1;
    }
    return 0;
}
