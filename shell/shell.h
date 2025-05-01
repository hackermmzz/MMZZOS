#ifndef MMZZ_SHELL_H
#define MMZZ_SHELL_H
#include"../fs/file.h"
/////////////////////////////////
#define CMD_MAX_LEN 256 
#define CMD_MAX_ARGUMENT_CNT 32
/////////////////////////////////
extern char cmd_buf[CMD_MAX_LEN+1];
extern char cmd_cwd[MAX_PATH_LEN+1];
extern char* cmd_argv[CMD_MAX_ARGUMENT_CNT+1];
/////////////////////////////////
void shell();
/////////////////////////////////
#endif