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
int  CmdParse(char*cmd_buf,char flag){
    int cnt=0;
    for(int i=0;i<CMD_MAX_ARGUMENT_CNT;++i)cmd_argv[i]=0;
    //
    char*buf=cmd_buf;
    while(*buf){
        if(*buf==flag){
            ++buf;
            continue;
        }
        else{
            if(cnt==CMD_MAX_ARGUMENT_CNT)return -1;
            cmd_argv[cnt++]=buf;
            while(*buf!=flag&&*buf)++buf;
            if(*buf)*buf=0,++buf;//如果是切割符，把它变成0，这样便于后续拷贝参数等
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
void CMD_Exec(int argc,char*argv[]){
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
}
bool ProcessExternal_CMD(int argc,char*argv[]){
    int32_t pid=fork();
    if(pid){
        int status;
        int chPid=wait(&status);
        ASSERT(chPid==pid);
    }else{
        CMD_Exec(argc,argv);
    }
    return 1;
}

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
void ProcessPipeCMD(){
    //计算需要创建的进程个数(进程只能是外部命令)
    int cnt=0;
    char*ss=cmd_buf;while(*ss)cnt+=(*ss=='|'),++ss;
    if(cnt!=1){
        printf("because of the power of wlh,he can't deal the problem to run more than 2 process using pipe,sorry!\n");
        return;
    }
    //
    int fd[2];
    if(pipe(fd)==0){
        printf("error occur when allocate pipe!\n");
        return;
    }
    //解析获得两个命令
    char*flag=(char*)strchr(cmd_buf,'|');
    *flag=0;
    char**argv0=malloc(sizeof(cmd_argv));
    char**argv1=malloc(sizeof(cmd_argv));
    if(!argv0||!argv1){
        printf("error occur when copy arguments!\n");
        if(argv0)free(argv0);
        if(argv1)free(argv1);
        return;
    }
    
    int argc0=CmdParse(cmd_buf,' ');
    memcpy(argv0,cmd_argv,sizeof(cmd_argv));
    int argc1=CmdParse(flag+1,' ');
    memcpy(argv1,cmd_argv,sizeof(cmd_argv));
    if(argc0==-1||argc1==-1){
        free(argv0);
        free(argv1);
        printf("error occur when parse arguments!\n");
        return;
    }
    //运行进程
    if(!fork()){
        fd_redirect(STDOUT_FD,fd[1]);//将第一个进程的输出流重定向
        close(fd[0]);//因为不需要fd[0],直接关闭剩下其他的描述符以备使用
        CMD_Exec(argc0,argv0);
    }
    if(!fork()){
        fd_redirect(STDIN_FD,fd[0]);//将第二个进程的输入流重定向
        close(fd[1]);
        CMD_Exec(argc1,argv1);
    }
    fd_redirect(STDIN_FD,STDIN_FD);
    fd_redirect(STDOUT_FD,STDOUT_FD);
    for(int i=0,status;i<2;++i){
        pid_t pid=wait(&status);
        if(pid==-1){
            printf("error child pid!\n");
            --i;
        }
    }
    //
    close(fd[0]);
    close(fd[1]);
}
void ShellExec(){
    //如果含有'|'当管道处理
    if(strchr(cmd_buf,'|')){
        ProcessPipeCMD();
        return;
    }
    //解析字符
    int argc=CmdParse(cmd_buf,' ');
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
