#ifndef MMZZ_BUILDIN_CMD_H
#define MMZZ_BUILDIN_CMD_H
#include"../lib/user/syscall.h"
//////////////////////////////////////
void GetAbsolutePath(const char*path,char*buf);
//////////////////////////////////////实现一些简单的内部命令
void buildin_ls(int argc,char**argv);//我先只实现不带参数版本的
void buildin_cd(int argc,char**argv);
void buildin_mkdir(int argc,char**argv);
void buildin_rmdir(int argc,char**argv);
void buildin_rm(int argc,char**argv);
void buildin_pwd(int argc,char**argv);
void buildin_ps(int argc,char**argv);
void buildin_clear(int argc,char**argv);
void buildin_echo(int argc,char**argv);
//////////////////////////////////////
#endif