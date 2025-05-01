#include "buildin_cmd.h"
#include"../fs/file.h"
#include"../lib/math.h"
#include"../lib/stdio.h"
#include"shell.h"
#include"../userprog/process.h"
//将path转成绝对路径保存在buf里面,并且去掉所有的.和..
void GetAbsolutePath(const char *path, char *buf)
{
    char tmp[MAX_PATH_LEN+1];tmp[0]=0;
    //判断path是否是绝对路径
    if(path[0]!='/'){
        if(getcwd(tmp,MAX_PATH_LEN)){
            //如果当前目录不是根目录
            if(!(tmp[0]=='/'&&tmp[1]==0)){
                strcat(tmp,"/");
            }
        }
    }
    strcat(tmp,path);//得到绝对路径
    //去掉路径中的.和..
    char* stack[MAX_PATH_LEN];
    int ptr=0;
    char*str=tmp;
    while(*str){
        if(*str=='/'){
            ++str;
            continue;
        }
        else{
            stack[ptr++]=str;
            while(*str&&*str!='/')++str;
            if(*str)*str=0,++str;
            //开始栈模拟弹出
            if(!strcmp(stack[ptr-1],"."))--ptr;
            else if(!strcmp(stack[ptr-1],".."))ptr=max(ptr-2,0);
            
        }
    }
    //拼接起来
    *buf=0;
    char*tmpp=buf;
    for(int i=0;i<ptr;++i){
        *tmpp='/';++tmpp;
        tmpp=strcpy(tmpp,stack[i]);
    }
    if(*buf==0)*buf='/',buf[1]=0;
    
}
//递归性的删除整个文件夹(包括文件夹本身),path为绝对路径
bool DirectoryRemove(const char*path,char*buf){
    char*buf1=malloc(MAX_PATH_LEN+1);
    if(buf1==0)return 0;
    struct Dir*dir=opendir(path);
    if(dir==0)return 0;
    struct DirEntry*entry;
    //
    bool flag=1;
    while(flag&&(entry=readdir(dir))){
        //如果是文件直接删除
        if(entry->filetype==FT_FILE){
            strcpy(buf,path);
            strcat(buf,"/");
            strcat(buf,entry->filename);
            unlink(buf);
        }
        //如果是目录,递归删除
        else if(entry->filetype==FT_DIR&&strcmp(entry->filename,".")&&strcmp(entry->filename,"..")){
            strcpy(buf1,path);
            strcat(buf1,"/");
            strcat(buf1,entry->filename);
            flag=DirectoryRemove(buf1,buf);
        }
    }
    //
    closedir(dir);
    free(buf1);
    if(flag)flag=rmdir(path);
    return flag;
}
void buildin_ls(int argc,char**argv)
{
    struct Dir*dir=opendir(cmd_cwd);
    ASSERT(dir);//dir是不可能为0的
    struct DirEntry*entry;
    while(entry=readdir(dir)){
        printf("%s   ",entry->filename);
    }
    putchar('\n');
    closedir(dir);
}

void buildin_cd(int argc,char**argv)
{
    //如果只是cd,那么他表示切换到根目录
    if(argc==0){
        chdir("/");
        return;
    }
    //否则切换到对应目录
    char path[MAX_PATH_LEN+1];
    GetAbsolutePath(argv[0],path);
    bool success=chdir(path);
    if(!success){
        printf("don't exist a folder named %s\n",argv[0]);
        return;
    }
    //更改shell当前的路径
    strcpy(cmd_cwd,path);
}

void buildin_mkdir(int argc, char **argv)
{
    if(argc==0)return;
    char*dir=argv[0];
    char path[MAX_PATH_LEN+1];
    GetAbsolutePath(dir,path);//获取绝对路径
    bool success=mkdir(path);
    if(!success){
        printf("mkdir failed!\n");
    }
}

void buildin_rmdir(int argc, char **argv)
{
    if(argc==0)return;
    char*dir=argv[0];
    char path[MAX_PATH_LEN+1];
    GetAbsolutePath(dir,path);
    bool success=rmdir(path);
    if(!success){
        printf("rmdir failed!\n");
    }
}

void buildin_rm(int argc, char **argv)
{
    if(argc==0)return;
    char*arg0=argv[0];
    char path[MAX_PATH_LEN+1];
    //如果要删除文件夹
    if(!strcmp(arg0,"-r")){
        if(argc>=2){
            char*arg1=argv[1];
            GetAbsolutePath(arg1,path);
            char buf[MAX_PATH_LEN+1];
            bool success=DirectoryRemove(path,buf);
            if(!success)printf("rm %s failed!\n",arg0);
        }
    }
    //删除普通文件
    else{
        GetAbsolutePath(arg0,path);
        bool success=unlink(path);
        if(!success)printf("rm %s failed!\n",arg0);
    }
}

void buildin_pwd(int argc, char **argv)
{
    printf("%s\n",cmd_cwd);
}

void buildin_ps(int argc, char **argv)
{
    //这里我就假装最多运行1024个进程了,此决定与syscall_sleep一样
    struct ProcessInfo*info=malloc(1024*sizeof(struct ProcessInfo));
    if(info==0)return;
    //
    int cnt=ps(info);
    char buf[256];
    //
    printf("name             pid    parent   status\n");
    for(int i=0;i<cnt;++i){
        struct ProcessInfo*p=&info[i];
        //打印进程名字,16位置对齐
        int nameLen=strlen(p->name);
        printf("%s",p->name);
        for(int i=0;i+nameLen<=THREAD_NAME_MAX_LEN;++i)putchar(' ');
        //打印进程pid,6位对齐
        sprintf(buf,"%d",p->pid);
        int pidLen=strlen(buf);
        printf("%s",buf);
        for(int i=0;i+pidLen<=6;++i)putchar(' ');
        //打印进程父进程pid,6位对齐
        if(p->parent!=(pid_t)-1)sprintf(buf,"%d",p->parent);
        else strcpy(buf,"none");
        pidLen=strlen(buf);
        printf("%s",buf);
        for(int i=0;i+pidLen<=6;++i)putchar(' ');
        //
        putchar(' ');putchar(' ');
        //打印进程当前状态
        int status=p->status;
        if(status==RUNNING)printf("Running");
        else if(status==READY)printf("Ready");
        else if(status==BLOCKED)printf("Blcoked");
        //
        putchar('\n');
    }
    //
    free(info);
}

void buildin_clear(int argc,char**argv)
{
    clear();
}
