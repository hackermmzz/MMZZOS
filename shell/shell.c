#include"shell.h"
#include"../lib/stdio.h"
#include"../lib/user/syscall.h"
#include"../device/keyboard.h"
#include"../userprog/process.h"
#include"../userprog/exec.h"
#include"buildin_cmd.h"
char cmd_buf[CMD_MAX_LEN+1];
char cmd_cwd[MAX_PATH_LEN+1];//当前shell工作所在的目录
char* cmd_argv[CMD_MAX_ARGUMENT_CNT+1];//参数
///////////////////////////////////////////////
void shellShowWelcome(){
    printf("welcome to the MMZZOS powered by LiHongWang!\n");
}
void shellTipShow(){
    printf("mmzz@MMZZOS:%s$",cmd_cwd);
}
int readline(void *buf,int32_t count)
{
    char*str=(char*)buf;
    int len=0;
    unsigned char ch;
    while(len<count){
        syscall(SYSCALL_READ,STDIN_FD,&ch,1);
        //处理ctrl+L
        if(ch==CODE_CTRL_L){
            //清屏
            clear();
            //输出提示
            shellTipShow();
            //输出已经输入的字符
            *str=0;
            printf("%s",buf);
        }
        //处理ctrl+U
        else if(ch==CODE_CTRL_U){
            while(len)putchar('\b'),--len;
            str=(char*)buf;
        }
        //处理换行
        else if(ch=='\n'){
            *str=0;
            putchar('\n');//换行
            break;
        }
        //处理退格
        else if(ch=='\b'){
            if(len){
                --len;
                --str;
                putchar('\b');
            }
        }
        //普通字符
        else{
            *str=ch;
            ++str;
            ++len;
            putchar(ch);
        }
    }
    return len;
}
int shellCmdParse(){
    int cnt=0;
    for(int i=0;i<CMD_MAX_ARGUMENT_CNT;++i)cmd_argv[i]=0;
    //
    char*buf=cmd_buf;
    while(*buf){
        if(*buf==' '){
            ++buf;
            continue;
        }
        else{
            if(cnt==CMD_MAX_ARGUMENT_CNT)return -1;
            cmd_argv[cnt++]=buf;
            while(*buf!=' '&&*buf)++buf;
            if(*buf)*buf=0,++buf;//如果是空格，把它变成0，这样便于后续拷贝参数等
        }
    }
    return cnt;
}
bool ProcessBuildin_CMD(int argc,char**argv){
    char*cmdName=argv[0];
    //
    if(!strcmp(cmdName,"clear")){
        buildin_clear(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"ls")){
        buildin_ls(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"cd")){
        buildin_cd(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"mkdir")){
        buildin_mkdir(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"rmdir")){
        buildin_rmdir(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"rm")){
        buildin_rm(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"pwd")){
        buildin_pwd(argc-1,argv+1);
        return 1;
    }
    else if(!strcmp(cmdName,"ps")){
        buildin_ps(argc-1,argv+1);
        return 1;
    }
    return 0;
}

bool ProcessExternal_CMD(int argc,char*argv[]){
    int32_t pid=fork();
    if(pid){
        while(1);
    }else{
        char path[MAX_PATH_LEN+1];
        GetAbsolutePath(argv[0],path);
        //先判断文件是否存在
        struct stat state;
        memset(&state,0,sizeof(state));
        stat(path,&state);
        if(state.filetype!=FT_FILE){
            printf("Error:can't access %s!\n",argv[0]);
        }else{
            exec(path,argc,argv);
        }
        while(1);
    }
    return 1;
}
void ShellExec(){
    //解析字符
    int argc=shellCmdParse();
    if(argc==-1){
        printf("You input more than %d arguments and it is not allowed!\n",CMD_MAX_ARGUMENT_CNT);
        return;
    }
    if(argc==0)return;
    ////处理命令
    //如果是内部命令
    if(ProcessBuildin_CMD(argc,cmd_argv))return;
    //反之为外部命令
    if(ProcessExternal_CMD(argc,cmd_argv))return;
    //反之找不到命令
    printf("Can't find command named: %s\n",cmd_argv[0]);
}

void shell()
{
    clear();//清屏
    shellShowWelcome();//打印初次登陆的一些信息
    memset(cmd_cwd,0,sizeof(cmd_cwd));//
    cmd_cwd[0]='/';//默认工作目录是根目录
    while(1){
        shellTipShow();
        int cmd_len=readline(cmd_buf,CMD_MAX_LEN);
        if(cmd_len){
            ShellExec();
        }
    }
}
