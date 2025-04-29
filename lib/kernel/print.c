#include "print.h"
#include "stdarg.h"
void put_str(const char*str){
    while(*str){
        put_char(*str);
        ++str;
    }
}

void put_num(int num){
    if(num==0){
        put_char('0');
        return;
    }
    if(num<0)
    {
        num=-num;
        put_char('-');
    }
    char buffer[20];//最多20位
    int idx=19;
    do{
        buffer[idx--]=num%10+'0';
        num/=10;
    }while(num);
    while(++idx<=19){
        put_char(buffer[idx]);
    }
}

void put_hex(uint32_t num)
{
    char buffer[8];
    for(int i=0;i<8;++i){
        int t=num&0b1111;
        buffer[i]=t<10?(t+'0'):(t-10+'a');
        num>>=4;
    }
    put_char('0');
    put_char('x');
    for(int i=7;i>=0;--i)put_char(buffer[i]);
}

void printk(const char*format,...){
    extern int vfprintf(char *, const char *, va_list );
    va_list arg;
    va_start(arg,format);
    char buf[100];
    vfprintf(buf,format,arg);
    put_str(buf);
    va_end(arg);
}