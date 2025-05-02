#include "../lib/stdint.h"
#include"../lib/stdio.h"
int main(int argc,char*argv[]){
    const char*str="SUCCESSWLH\n";
    write(0,str,11);
    printf("argc: %d\n",argc);
    for(int i=0;i<argc;++i){
       printf("%s\n",argv[i]);
    }
    while(1);
    return 0;
}