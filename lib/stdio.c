#include "stdio.h"
#include"../fs/file.h"
//按进制拆分value
void itoa(uint32_t value,char**buf,int base){
    if(value==0){
        **buf='0';
        ++(*buf);
        return;
    }
    else{
        int wb=value%base;
        value/=base;
        if(value)itoa(value,buf,base);
        **buf=(wb<=9?wb+'0':wb-10+'a');
        ++(*buf);
    }
}
int vfprintf(char *buf, const char *format, va_list arg)
{
    char*ptr=buf;
    const char*idx=format;
    while(*idx){
        if(*idx=='%'){
            ++idx;
            if(*idx=='d'){
                int32_t value=va_arg(arg,int32_t);
                if(value<0){
                    *buf='-';
                    ++buf;
                    value=-value;
                }
                itoa(value,&buf,10);
            }else if(*idx=='x'){
                uint32_t value=va_arg(arg,uint32_t);
                *buf='0';++buf;*buf='x';++buf;
                itoa(value,&buf,16);
            }else if(*idx=='b'){
                uint32_t value=va_arg(arg,uint32_t);
                *buf='0';++buf;*buf='b';++buf;
                itoa(value,&buf,2);
            }else if(*idx=='o'){
                uint32_t value=va_arg(arg,uint32_t);
                *buf='0';++buf;
                itoa(value,&buf,8);
            }else if(*idx=='s'){
                const char*str=va_arg(arg,const char*);
                buf=strcpy(buf,str);
            }else if(*idx=='c'){
                *buf=(char)(va_arg(arg,int));
                ++buf;
            }
        }else{
            *buf=*idx;
            ++buf;
        }
        ++idx;
    }
    *buf=0;
    return strlen(buf);
}
int printf(const char*format,...){
    va_list arg;
    va_start(arg,format);
    char buf[1024];
    int len=vfprintf(buf,format,arg);
    syscall(SYSCALL_WRITE,STDOUT_FD,buf,len);
    va_end(arg);
    return len;
}

int sprintf(char *buf, const char *format, ...)
{
    va_list arg;
    va_start(arg,format);
    int len=vfprintf(buf,format,arg);
    va_end(arg);
    return len;
}
