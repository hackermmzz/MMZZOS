#include"../lib/user/syscall.h"
#include"../lib/stdio.h"
#include"../lib/string.h"
#include"../fs/file.h"
int main(int argc,char**argv){
    if(argc>=2){
        char*file=argv[1];
        int fd=open(file,O_RDONLY);
        if(fd==-1){
            printf("can't open file %s\n",file);
            return 0;
        }
        int32_t size=seek(fd,0,SEEK_END);
        seek(fd,0,SEEK_SET);
        char*buf=malloc(size);
        if(buf==0){
            printf("can't malloc buffer!\n");
            return 0;
        }
        int cnt=read(fd,buf,size);
        if(cnt!=size){
            printf("can't read file %s\n",file);
            return 0;
        }
        printf("%s\n",buf);
    }
    return 0;
}