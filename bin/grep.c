#include"../lib/user/syscall.h"
#include"../lib/stdio.h"
#include"../lib/string.h"
#include"../fs/file.h"
#define MAX_BUFFER_SIZE 1024
//查看str0是否出现过str1
bool str0Occurstr1(const char*str0,const char*str1){
    int len0=strlen(str0);
    int len1=strlen(str1);
    for(int i=0;i+len1-1<len0;++i){
        bool flag=1;
        for(int j=0;j<len1;++j){
            if(str0[i+j]!=str1[j]){
                flag=0;
                break;
            }
        }
        if(flag)return 1;
    }
    return 0;
}
int main(int argc,char**argv){
    char*buf=malloc(MAX_BUFFER_SIZE+1);
    char*target=argv[1];
    if(buf){
        //
        int cnt;
        int line=0;
        while(1){
            int cnt=getline(buf,MAX_BUFFER_SIZE);
            if(cnt==0){
                sleep(3000);
                cnt=getline(buf,MAX_BUFFER_SIZE);
                if(cnt==0)return 0;
            }
            ++line;
            if(str0Occurstr1(buf,target)){
                printf("%d:%s\n",line,buf);
            }
        }
        //
        free(buf);
    }
    return 0;
}